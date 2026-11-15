/** GopRotationHook: logical landscape GOP backed by a software-rotated shadow.
 *  This hook is active only while UEFI Boot Services are available. It does
 *  not rotate direct framebuffer writes and does not survive ExitBootServices.
 */
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/GraphicsOutput.h>

#define MAX_GOPS 8

typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT OriginalBlt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE OriginalQueryMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE OriginalSetMode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *PhysicalInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *LogicalInfo;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Shadow;
  UINT32 PhysicalWidth;
  UINT32 PhysicalHeight;
  UINT32 LogicalWidth;
  UINT32 LogicalHeight;
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

STATIC BOOLEAN
RectValid(UINTN X, UINTN Y, UINTN Width, UINTN Height, UINTN LimitW, UINTN LimitH)
{
  return (BOOLEAN)(X <= LimitW && Y <= LimitH &&
                   Width <= LimitW - X && Height <= LimitH - Y);
}

STATIC EFI_STATUS
AllocateShadow(GOP_CONTEXT *Context)
{
  UINTN Pixels;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *NewShadow;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *NewInfo;

  if (Context->PhysicalWidth == 0 || Context->PhysicalHeight == 0 ||
      Context->PhysicalWidth > MAX_UINTN / Context->PhysicalHeight) {
    return EFI_UNSUPPORTED;
  }
  Pixels = (UINTN)Context->PhysicalWidth * Context->PhysicalHeight;
  if (Pixels > MAX_UINTN / sizeof(*NewShadow)) return EFI_OUT_OF_RESOURCES;

  NewShadow = AllocateZeroPool(Pixels * sizeof(*NewShadow));
  if (NewShadow == NULL) return EFI_OUT_OF_RESOURCES;
  NewInfo = AllocateCopyPool(sizeof(*NewInfo), Context->PhysicalInfo);
  if (NewInfo == NULL) {
    FreePool(NewShadow);
    return EFI_OUT_OF_RESOURCES;
  }

  NewInfo->HorizontalResolution = Context->PhysicalHeight;
  NewInfo->VerticalResolution = Context->PhysicalWidth;
  NewInfo->PixelsPerScanLine = Context->PhysicalHeight;

  if (Context->Shadow != NULL) FreePool(Context->Shadow);
  if (Context->LogicalInfo != NULL) FreePool(Context->LogicalInfo);
  Context->Shadow = NewShadow;
  Context->LogicalInfo = NewInfo;
  Context->LogicalWidth = Context->PhysicalHeight;
  Context->LogicalHeight = Context->PhysicalWidth;
  if (Context->Gop->Mode != NULL) {
    Context->Gop->Mode->Info = Context->LogicalInfo;
    Context->Gop->Mode->SizeOfInfo = sizeof(*Context->LogicalInfo);
  }
  return EFI_SUCCESS;
}

/* Flush logical landscape shadow to physical portrait panel, clockwise. */
STATIC EFI_STATUS
FlushShadow(GOP_CONTEXT *Context)
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Physical;
  UINTN X, Y, Count;
  EFI_STATUS Status;

  Count = (UINTN)Context->PhysicalWidth * Context->PhysicalHeight;
  Physical = AllocatePool(Count * sizeof(*Physical));
  if (Physical == NULL) return EFI_OUT_OF_RESOURCES;

  for (Y = 0; Y < Context->LogicalHeight; ++Y) {
    for (X = 0; X < Context->LogicalWidth; ++X) {
      UINTN Px = Context->PhysicalWidth - 1 - Y;
      UINTN Py = X;
      Physical[Py * Context->PhysicalWidth + Px] =
        Context->Shadow[Y * Context->LogicalWidth + X];
    }
  }

  Status = Context->OriginalBlt(Context->Gop, Physical, EfiBltBufferToVideo,
                                0, 0, 0, 0, Context->PhysicalWidth,
                                Context->PhysicalHeight,
                                Context->PhysicalWidth * sizeof(*Physical));
  FreePool(Physical);
  return Status;
}

STATIC EFI_STATUS EFIAPI
HookQueryMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *This, UINT32 ModeNumber,
              UINTN *SizeOfInfo, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info)
{
  GOP_CONTEXT *Context = FindContext(This);
  EFI_STATUS Status;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *ModeInfo = NULL;
  UINTN InfoSize = 0;

  if (Context == NULL || SizeOfInfo == NULL || Info == NULL) return EFI_INVALID_PARAMETER;
  Status = Context->OriginalQueryMode(This, ModeNumber, &InfoSize, &ModeInfo);
  if (EFI_ERROR(Status)) return Status;
  if (ModeInfo == NULL || InfoSize < sizeof(*ModeInfo)) {
    if (ModeInfo != NULL) FreePool(ModeInfo);
    return EFI_DEVICE_ERROR;
  }
  {
    UINT32 Temp = ModeInfo->HorizontalResolution;
    ModeInfo->HorizontalResolution = ModeInfo->VerticalResolution;
    ModeInfo->VerticalResolution = Temp;
    ModeInfo->PixelsPerScanLine = ModeInfo->HorizontalResolution;
  }
  *SizeOfInfo = InfoSize;
  *Info = ModeInfo;
  return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
HookSetMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *This, UINT32 ModeNumber)
{
  GOP_CONTEXT *Context = FindContext(This);
  EFI_STATUS Status;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *PhysicalInfo;

  if (Context == NULL || This->Mode == NULL) return EFI_UNSUPPORTED;

  /* Restore the physical mode-info view before calling the real GOP method. */
  if (Context->PhysicalInfo != NULL) This->Mode->Info = Context->PhysicalInfo;
  Status = Context->OriginalSetMode(This, ModeNumber);
  if (EFI_ERROR(Status)) return Status;
  if (This->Mode == NULL || This->Mode->Info == NULL) return EFI_DEVICE_ERROR;

  PhysicalInfo = AllocateCopyPool(sizeof(*PhysicalInfo), This->Mode->Info);
  if (PhysicalInfo == NULL) return EFI_OUT_OF_RESOURCES;
  if (Context->PhysicalInfo != NULL) FreePool(Context->PhysicalInfo);
  Context->PhysicalInfo = PhysicalInfo;
  Context->PhysicalWidth = PhysicalInfo->HorizontalResolution;
  Context->PhysicalHeight = PhysicalInfo->VerticalResolution;

  Status = AllocateShadow(Context);
  if (EFI_ERROR(Status)) return Status;
  DEBUG((DEBUG_INFO, "GopRotationHook: SetMode %u => logical %ux%u\n",
         ModeNumber, Context->LogicalWidth, Context->LogicalHeight));
  return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
HookBlt(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
        EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
        UINTN SourceX, UINTN SourceY, UINTN DestinationX, UINTN DestinationY,
        UINTN Width, UINTN Height, UINTN Delta)
{
  GOP_CONTEXT *Context = FindContext(This);
  EFI_STATUS Status = EFI_SUCCESS;
  UINTN X, Y, Stride;

  if (Context == NULL || Context->Shadow == NULL) return EFI_NOT_READY;
  if (Width == 0 || Height == 0) return EFI_SUCCESS;

  switch (BltOperation) {
    case EfiBltVideoFill:
      if (BltBuffer == NULL ||
          !RectValid(DestinationX, DestinationY, Width, Height,
                     Context->LogicalWidth, Context->LogicalHeight)) {
        return EFI_INVALID_PARAMETER;
      }
      for (Y = 0; Y < Height; ++Y) {
        for (X = 0; X < Width; ++X) {
          Context->Shadow[(DestinationY + Y) * Context->LogicalWidth + DestinationX + X] =
            BltBuffer[0];
        }
      }
      return FlushShadow(Context);

    case EfiBltBufferToVideo:
      if (BltBuffer == NULL ||
          !RectValid(DestinationX, DestinationY, Width, Height,
                     Context->LogicalWidth, Context->LogicalHeight)) {
        return EFI_INVALID_PARAMETER;
      }
      Stride = (Delta == 0) ? (SourceX + Width) * sizeof(*BltBuffer) : Delta;
      if (Stride < (SourceX + Width) * sizeof(*BltBuffer) ||
          SourceY > MAX_UINTN / Stride) return EFI_INVALID_PARAMETER;
      for (Y = 0; Y < Height; ++Y) {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Row =
          (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)BltBuffer +
                                            (SourceY + Y) * Stride);
        CopyMem(&Context->Shadow[(DestinationY + Y) * Context->LogicalWidth + DestinationX],
                &Row[SourceX], Width * sizeof(*BltBuffer));
      }
      return FlushShadow(Context);

    case EfiBltVideoToBltBuffer:
      if (BltBuffer == NULL ||
          !RectValid(SourceX, SourceY, Width, Height,
                     Context->LogicalWidth, Context->LogicalHeight)) {
        return EFI_INVALID_PARAMETER;
      }
      Stride = (Delta == 0) ? (DestinationX + Width) * sizeof(*BltBuffer) : Delta;
      if (Stride < (DestinationX + Width) * sizeof(*BltBuffer) ||
          DestinationY > MAX_UINTN / Stride) return EFI_INVALID_PARAMETER;
      for (Y = 0; Y < Height; ++Y) {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Row =
          (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)BltBuffer +
                                            (DestinationY + Y) * Stride);
        CopyMem(&Row[DestinationX],
                &Context->Shadow[(SourceY + Y) * Context->LogicalWidth + SourceX],
                Width * sizeof(*BltBuffer));
      }
      return EFI_SUCCESS;

    case EfiBltVideoToVideo:
      if (!RectValid(SourceX, SourceY, Width, Height,
                     Context->LogicalWidth, Context->LogicalHeight) ||
          !RectValid(DestinationX, DestinationY, Width, Height,
                     Context->LogicalWidth, Context->LogicalHeight)) {
        return EFI_INVALID_PARAMETER;
      }
      {
        UINTN Bytes = Width * Height * sizeof(*Context->Shadow);
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Temp;
        if (Width > MAX_UINTN / Height ||
            Width * Height > MAX_UINTN / sizeof(*Temp)) return EFI_OUT_OF_RESOURCES;
        Temp = AllocatePool(Bytes);
        if (Temp == NULL) return EFI_OUT_OF_RESOURCES;
        for (Y = 0; Y < Height; ++Y) {
          CopyMem(&Temp[Y * Width],
                  &Context->Shadow[(SourceY + Y) * Context->LogicalWidth + SourceX],
                  Width * sizeof(*Temp));
        }
        for (Y = 0; Y < Height; ++Y) {
          CopyMem(&Context->Shadow[(DestinationY + Y) * Context->LogicalWidth + DestinationX],
                  &Temp[Y * Width], Width * sizeof(*Temp));
        }
        FreePool(Temp);
      }
      return FlushShadow(Context);

    default:
      Status = EFI_UNSUPPORTED;
      break;
  }
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

  Context = &mContexts[mContextCount];
  ZeroMem(Context, sizeof(*Context));
  Context->Gop = Gop;
  Context->OriginalBlt = Gop->Blt;
  Context->OriginalQueryMode = Gop->QueryMode;
  Context->OriginalSetMode = Gop->SetMode;
  Context->PhysicalInfo = AllocateCopyPool(sizeof(*Context->PhysicalInfo), Gop->Mode->Info);
  if (Context->PhysicalInfo == NULL) return EFI_OUT_OF_RESOURCES;
  Context->PhysicalWidth = Context->PhysicalInfo->HorizontalResolution;
  Context->PhysicalHeight = Context->PhysicalInfo->VerticalResolution;
  Status = AllocateShadow(Context);
  if (EFI_ERROR(Status)) {
    FreePool(Context->PhysicalInfo);
    ZeroMem(Context, sizeof(*Context));
    return Status;
  }

  ++mContextCount;
  Gop->Blt = HookBlt;
  Gop->QueryMode = HookQueryMode;
  Gop->SetMode = HookSetMode;
  Context->Installed = TRUE;
  DEBUG((DEBUG_INFO, "GopRotationHook: hooked GOP %p physical %ux%u logical %ux%u\n",
         Gop, Context->PhysicalWidth, Context->PhysicalHeight,
         Context->LogicalWidth, Context->LogicalHeight));
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
  DEBUG((DEBUG_INFO, "GopRotationHook: initialized; software GOP rotation active\n"));
  return EFI_SUCCESS;
}
