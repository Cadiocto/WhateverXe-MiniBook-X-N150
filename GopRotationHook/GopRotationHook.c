/** GopRotationHook: experimental UEFI GOP rotation hook.
 *  UEFI Boot Services only; this does not provide post-ExitBootServices support.
 */
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Protocol/GraphicsOutput.h>

#define MAX_GOPS 8

typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT OriginalBlt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE OriginalQueryMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE OriginalSetMode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *LogicalInfo;
  UINT32 PhysicalWidth;
  UINT32 PhysicalHeight;
  BOOLEAN Installed;
} GOP_CONTEXT;

STATIC GOP_CONTEXT mContexts[MAX_GOPS];
STATIC UINTN mContextCount;
STATIC EFI_EVENT mNotifyEvent;
STATIC VOID *mNotifyRegistration;

STATIC GOP_CONTEXT *
FindContext(EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop)
{
  UINTN Index;
  for (Index = 0; Index < mContextCount; ++Index) {
    if (mContexts[Index].Gop == Gop) return &mContexts[Index];
  }
  return NULL;
}

STATIC EFI_STATUS EFIAPI
HookQueryMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *This, UINT32 ModeNumber,
              UINTN *SizeOfInfo, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info)
{
  GOP_CONTEXT *Context = FindContext(This);
  EFI_STATUS Status;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *OriginalInfo = NULL;
  UINTN OriginalSize = 0;

  if (Context == NULL) return EFI_UNSUPPORTED;
  Status = Context->OriginalQueryMode(This, ModeNumber, &OriginalSize, &OriginalInfo);
  if (EFI_ERROR(Status)) return Status;
  if (OriginalInfo == NULL || SizeOfInfo == NULL || Info == NULL) {
    if (OriginalInfo != NULL) FreePool(OriginalInfo);
    return EFI_INVALID_PARAMETER;
  }
  OriginalInfo->HorizontalResolution = Context->PhysicalHeight;
  OriginalInfo->VerticalResolution = Context->PhysicalWidth;
  *SizeOfInfo = OriginalSize;
  *Info = OriginalInfo;
  return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
HookSetMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *This, UINT32 ModeNumber)
{
  GOP_CONTEXT *Context = FindContext(This);
  EFI_STATUS Status;
  if (Context == NULL) return EFI_UNSUPPORTED;
  Status = Context->OriginalSetMode(This, ModeNumber);
  if (!EFI_ERROR(Status) && This->Mode != NULL && This->Mode->Info != NULL) {
    Context->PhysicalWidth = This->Mode->Info->HorizontalResolution;
    Context->PhysicalHeight = This->Mode->Info->VerticalResolution;
  }
  return Status;
}

/* The wrapper deliberately preserves unsupported BLT modes rather than
 * corrupting GOP buffers. BufferToVideo 32-bit BLTs are rotated clockwise.
 */
STATIC EFI_STATUS EFIAPI
HookBlt(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
        EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
        UINTN SourceX, UINTN SourceY, UINTN DestinationX, UINTN DestinationY,
        UINTN Width, UINTN Height, UINTN Delta)
{
  GOP_CONTEXT *Context = FindContext(This);
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Rotated;
  EFI_STATUS Status;
  UINTN X, Y, OutWidth, OutHeight, BufferDelta;

  if (Context == NULL) return EFI_UNSUPPORTED;
  if (BltOperation != EfiBltBufferToVideo || BltBuffer == NULL ||
      Width == 0 || Height == 0) {
    return Context->OriginalBlt(This, BltBuffer, BltOperation, SourceX, SourceY,
                                DestinationX, DestinationY, Width, Height, Delta);
  }

  /* A clockwise rotation maps a logical WxH source rectangle to HxW. */
  OutWidth = Height;
  OutHeight = Width;
  if (DestinationX > Context->PhysicalWidth ||
      DestinationY > Context->PhysicalHeight ||
      OutWidth > Context->PhysicalWidth - DestinationX ||
      OutHeight > Context->PhysicalHeight - DestinationY) {
    return EFI_INVALID_PARAMETER;
  }
  BufferDelta = (Delta == 0) ? Width * sizeof(*BltBuffer) : Delta;
  if (BufferDelta < (SourceX + Width) * sizeof(*BltBuffer)) {
    return EFI_INVALID_PARAMETER;
  }
  Rotated = AllocatePool(OutWidth * OutHeight * sizeof(*Rotated));
  if (Rotated == NULL) return EFI_OUT_OF_RESOURCES;

  for (Y = 0; Y < Height; ++Y) {
    for (X = 0; X < Width; ++X) {
      EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Source =
        (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)BltBuffer +
          (SourceY + Y) * BufferDelta) + SourceX + X;
      /* clockwise: (x,y) -> (H-1-y,x) */
      Rotated[X * OutWidth + (Height - 1 - Y)] = *Source;
    }
  }

  Status = Context->OriginalBlt(This, Rotated, EfiBltBufferToVideo,
                                0, 0, DestinationX, DestinationY,
                                OutWidth, OutHeight, OutWidth * sizeof(*Rotated));
  FreePool(Rotated);
  return Status;
}

STATIC EFI_STATUS
InstallHook(EFI_HANDLE Handle)
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  GOP_CONTEXT *Context;
  EFI_STATUS Status;

  Status = gBS->HandleProtocol(Handle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&Gop);
  if (EFI_ERROR(Status) || Gop == NULL || FindContext(Gop) != NULL) return Status;
  if (mContextCount >= MAX_GOPS || Gop->Mode == NULL || Gop->Mode->Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Context = &mContexts[mContextCount++];
  ZeroMem(Context, sizeof(*Context));
  Context->Gop = Gop;
  Context->OriginalBlt = Gop->Blt;
  Context->OriginalQueryMode = Gop->QueryMode;
  Context->OriginalSetMode = Gop->SetMode;
  Context->PhysicalWidth = Gop->Mode->Info->HorizontalResolution;
  Context->PhysicalHeight = Gop->Mode->Info->VerticalResolution;
  Gop->Blt = HookBlt;
  Gop->QueryMode = HookQueryMode;
  Gop->SetMode = HookSetMode;
  Context->Installed = TRUE;

  DEBUG((DEBUG_INFO, "GopRotationHook: hooked GOP %p, physical %ux%u\n",
         Gop, Context->PhysicalWidth, Context->PhysicalHeight));
  return EFI_SUCCESS;
}

STATIC VOID EFIAPI
GopNotify(EFI_EVENT Event, VOID *Context)
{
  EFI_STATUS Status;
  EFI_HANDLE Handle;
  UINTN BufferSize = sizeof(Handle);
  (VOID)Context;
  while (!EFI_ERROR(gBS->LocateHandle(ByRegisterNotify, NULL, mNotifyRegistration,
                                       &BufferSize, &Handle))) {
    Status = InstallHook(Handle);
    DEBUG((DEBUG_INFO, "GopRotationHook: notification install: %r\n", Status));
    BufferSize = sizeof(Handle);
  }
  (VOID)Event;
}

EFI_STATUS EFIAPI
GopRotationHookEntryPoint(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;
  EFI_HANDLE *Handles = NULL;
  UINTN HandleCount = 0, Index;
  (VOID)ImageHandle;
  (VOID)SystemTable;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid,
                                   NULL, &HandleCount, &Handles);
  if (!EFI_ERROR(Status)) {
    for (Index = 0; Index < HandleCount; ++Index) {
      Status = InstallHook(Handles[Index]);
      DEBUG((DEBUG_INFO, "GopRotationHook: existing GOP %u: %r\n", Index, Status));
    }
    FreePool(Handles);
  }

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, GopNotify, NULL, &mNotifyEvent);
  if (EFI_ERROR(Status)) return Status;
  Status = gBS->RegisterProtocolNotify(&gEfiGraphicsOutputProtocolGuid,
                                       mNotifyEvent, &mNotifyRegistration);
  if (EFI_ERROR(Status)) {
    gBS->CloseEvent(mNotifyEvent);
    return Status;
  }
  DEBUG((DEBUG_INFO, "GopRotationHook: initialized; UEFI-only experimental hook\n"));
  return EFI_SUCCESS;
}
