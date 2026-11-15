#include <Uefi.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include <Protocol/GraphicsOutput.h>

#define GOP_ROTATION_MAX_CONTEXTS  8

typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;

  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT        OriginalBlt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE OriginalQueryMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE   OriginalSetMode;

  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *PhysicalInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *LogicalInfo;

  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *SavedPhysicalModeInfo;
  UINTN SavedPhysicalSizeOfInfo;

  UINT32 PhysicalWidth;
  UINT32 PhysicalHeight;

  UINT32 LogicalWidth;
  UINT32 LogicalHeight;

  BOOLEAN Installed;
} GOP_ROTATION_CONTEXT;

typedef struct {
  UINTN X;
  UINTN Y;
  UINTN Width;
  UINTN Height;
} GOP_RECT;

STATIC GOP_ROTATION_CONTEXT mContexts[GOP_ROTATION_MAX_CONTEXTS];
STATIC UINTN mContextCount = 0;

STATIC EFI_EVENT mNotifyEvent = NULL;
STATIC VOID *mNotifyRegistration = NULL;

STATIC GOP_ROTATION_CONTEXT *
FindContext (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop
  )
{
  UINTN Index;
  if (Gop == NULL) return NULL;
  for (Index = 0; Index < mContextCount; Index++) {
    if (mContexts[Index].Gop == Gop) return &mContexts[Index];
  }
  return NULL;
}

STATIC GOP_ROTATION_CONTEXT *
AllocateContext (VOID)
{
  if (mContextCount >= GOP_ROTATION_MAX_CONTEXTS) return NULL;
  return &mContexts[mContextCount];
}

STATIC BOOLEAN
SafeMultiply (IN UINTN A, IN UINTN B, OUT UINTN *Result)
{
  if (Result == NULL) return FALSE;
  if (A != 0 && B > MAX_UINTN / A) return FALSE;
  *Result = A * B;
  return TRUE;
}

STATIC BOOLEAN
SafeAdd (IN UINTN A, IN UINTN B, OUT UINTN *Result)
{
  if (Result == NULL || B > MAX_UINTN - A) return FALSE;
  *Result = A + B;
  return TRUE;
}

STATIC BOOLEAN
RectValid (
  IN UINTN X, IN UINTN Y, IN UINTN Width, IN UINTN Height,
  IN UINTN LimitWidth, IN UINTN LimitHeight
  )
{
  if (X > LimitWidth || Y > LimitHeight) return FALSE;
  if (Width > LimitWidth - X) return FALSE;
  if (Height > LimitHeight - Y) return FALSE;
  return TRUE;
}

STATIC EFI_STATUS
TransformRectCW (
  IN GOP_ROTATION_CONTEXT *Context,
  IN UINTN LogicalX, IN UINTN LogicalY, IN UINTN LogicalWidth, IN UINTN LogicalHeight,
  OUT GOP_RECT *PhysicalRect
  )
{
  UINTN Right;
  if (Context == NULL || PhysicalRect == NULL) return EFI_INVALID_PARAMETER;
  if (!RectValid(LogicalX, LogicalY, LogicalWidth, LogicalHeight, Context->LogicalWidth, Context->LogicalHeight)) return EFI_INVALID_PARAMETER;
  if (!SafeAdd(LogicalY, LogicalHeight, &Right)) return EFI_INVALID_PARAMETER;
  if (Right > Context->PhysicalWidth) return EFI_INVALID_PARAMETER;

  PhysicalRect->X = Context->PhysicalWidth - Right;
  PhysicalRect->Y = LogicalX;
  PhysicalRect->Width = LogicalHeight;
  PhysicalRect->Height = LogicalWidth;

  if (!RectValid(PhysicalRect->X, PhysicalRect->Y, PhysicalRect->Width, PhysicalRect->Height, Context->PhysicalWidth, Context->PhysicalHeight)) return EFI_INVALID_PARAMETER;
  return EFI_SUCCESS;
}

STATIC EFI_STATUS
UpdateLogicalModeInfo (IN GOP_ROTATION_CONTEXT *Context)
{
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *NewInfo;
  if (Context == NULL || Context->PhysicalInfo == NULL || Context->Gop == NULL || Context->Gop->Mode == NULL) return EFI_INVALID_PARAMETER;

  NewInfo = AllocateCopyPool(sizeof(*NewInfo), Context->PhysicalInfo);
  if (NewInfo == NULL) return EFI_OUT_OF_RESOURCES;

  NewInfo->HorizontalResolution = Context->PhysicalHeight;
  NewInfo->VerticalResolution   = Context->PhysicalWidth;
  NewInfo->PixelsPerScanLine = Context->LogicalWidth;

  if (Context->LogicalInfo != NULL) FreePool(Context->LogicalInfo);

  Context->LogicalInfo = NewInfo;
  Context->LogicalWidth  = NewInfo->HorizontalResolution;
  Context->LogicalHeight = NewInfo->VerticalResolution;
  Context->Gop->Mode->Info = Context->LogicalInfo;
  Context->Gop->Mode->SizeOfInfo = sizeof(*Context->LogicalInfo);

  return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
HookQueryMode (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN UINT32 ModeNumber,
  OUT UINTN *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
  )
{
  GOP_ROTATION_CONTEXT *Context;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *PhysicalInfo;
  EFI_STATUS Status;
  UINTN PhysicalInfoSize;
  UINT32 Temp;

  if (This == NULL || SizeOfInfo == NULL || Info == NULL) return EFI_INVALID_PARAMETER;
  Context = FindContext(This);
  if (Context == NULL || Context->OriginalQueryMode == NULL) return EFI_UNSUPPORTED;

  *SizeOfInfo = 0;
  *Info = NULL;
  PhysicalInfo = NULL;
  PhysicalInfoSize = 0;

  Status = Context->OriginalQueryMode(This, ModeNumber, &PhysicalInfoSize, &PhysicalInfo);
  if (EFI_ERROR(Status)) return Status;

  if (PhysicalInfo == NULL || PhysicalInfoSize < sizeof(*PhysicalInfo)) {
    if (PhysicalInfo != NULL) FreePool(PhysicalInfo);
    return EFI_DEVICE_ERROR;
  }

  Temp = PhysicalInfo->HorizontalResolution;
  PhysicalInfo->HorizontalResolution = PhysicalInfo->VerticalResolution;
  PhysicalInfo->VerticalResolution = Temp;
  PhysicalInfo->PixelsPerScanLine = PhysicalInfo->HorizontalResolution;

  *SizeOfInfo = PhysicalInfoSize;
  *Info = PhysicalInfo;
  return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
HookSetMode (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN UINT32 ModeNumber
  )
{
  GOP_ROTATION_CONTEXT *Context;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *PhysicalInfo;
  EFI_STATUS Status;

  if (This == NULL || This->Mode == NULL) return EFI_INVALID_PARAMETER;
  Context = FindContext(This);
  if (Context == NULL || Context->OriginalSetMode == NULL || Context->PhysicalInfo == NULL) return EFI_UNSUPPORTED;

  This->Mode->Info = Context->SavedPhysicalModeInfo;
  This->Mode->SizeOfInfo = Context->SavedPhysicalSizeOfInfo;

  Status = Context->OriginalSetMode(This, ModeNumber);

  if (EFI_ERROR(Status)) {
    if (Context->LogicalInfo != NULL) {
      This->Mode->Info = Context->LogicalInfo;
      This->Mode->SizeOfInfo = sizeof(*Context->LogicalInfo);
    }
    return Status;
  }

  if (This->Mode == NULL || This->Mode->Info == NULL) return EFI_DEVICE_ERROR;

  Context->SavedPhysicalModeInfo = This->Mode->Info;
  Context->SavedPhysicalSizeOfInfo = This->Mode->SizeOfInfo;

  PhysicalInfo = AllocateCopyPool(sizeof(*PhysicalInfo), Context->SavedPhysicalModeInfo);
  if (PhysicalInfo == NULL) return EFI_OUT_OF_RESOURCES;

  if (Context->PhysicalInfo != NULL) FreePool(Context->PhysicalInfo);
  Context->PhysicalInfo = PhysicalInfo;
  Context->PhysicalWidth = PhysicalInfo->HorizontalResolution;
  Context->PhysicalHeight = PhysicalInfo->VerticalResolution;

  Status = UpdateLogicalModeInfo(Context);
  if (EFI_ERROR(Status)) return Status;

  DEBUG((DEBUG_INFO, "GopRotationHook: SetMode %u, physical %ux%u, logical %ux%u\n", ModeNumber, Context->PhysicalWidth, Context->PhysicalHeight, Context->LogicalWidth, Context->LogicalHeight));
  return EFI_SUCCESS;
}

STATIC EFI_STATUS
CalculateBufferStride (IN UINTN X, IN UINTN Width, IN UINTN Delta, OUT UINTN *Stride)
{
  UINTN Pixels;
  if (Stride == NULL) return EFI_INVALID_PARAMETER;
  if (!SafeAdd(X, Width, &Pixels) || !SafeMultiply(Pixels, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL), Stride)) return EFI_INVALID_PARAMETER;
  if (Delta != 0) {
    if (Delta < *Stride || Delta % sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) != 0) return EFI_INVALID_PARAMETER;
    *Stride = Delta;
  }
  return EFI_SUCCESS;
}

STATIC VOID
LogicalToPhysicalBuffer (
  IN CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Source, IN UINTN SourceStride,
  IN UINTN SourceX, IN UINTN SourceY, IN UINTN Width, IN UINTN Height,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Destination
  )
{
  UINTN X, Y;
  for (Y = 0; Y < Height; Y++) {
    CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *SourceRow = (CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((CONST UINT8 *)Source + (SourceY + Y) * SourceStride);
    for (X = 0; X < Width; X++) {
      UINTN PhysicalX = Height - 1 - Y;
      UINTN PhysicalY = X;
      Destination[PhysicalY * Height + PhysicalX] = SourceRow[SourceX + X];
    }
  }
}

STATIC VOID
PhysicalToLogicalBuffer (
  IN CONST EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Source, IN UINTN Width, IN UINTN Height,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Destination, IN UINTN DestinationStride,
  IN UINTN DestinationX, IN UINTN DestinationY
  )
{
  UINTN X, Y;
  for (Y = 0; Y < Height; Y++) {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *DestinationRow = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)((UINT8 *)Destination + (DestinationY + Y) * DestinationStride);
    for (X = 0; X < Width; X++) {
      UINTN PhysicalX = Height - 1 - Y;
      UINTN PhysicalY = X;
      DestinationRow[DestinationX + X] = Source[PhysicalY * Height + PhysicalX];
    }
  }
}

STATIC EFI_STATUS EFIAPI
HookBlt (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer OPTIONAL,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
  IN UINTN SourceX, IN UINTN SourceY, IN UINTN DestinationX, IN UINTN DestinationY,
  IN UINTN Width, IN UINTN Height, IN UINTN Delta
  )
{
  GOP_ROTATION_CONTEXT *Context;
  EFI_STATUS Status;
  GOP_RECT PhysicalRect;
  UINTN Stride, PixelCount, BufferBytes;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Temporary;

  if (This == NULL) return EFI_INVALID_PARAMETER;
  Context = FindContext(This);
  if (Context == NULL || Context->OriginalBlt == NULL) return EFI_UNSUPPORTED;
  if (Width == 0 || Height == 0) return EFI_SUCCESS;
  if (Context->PhysicalInfo == NULL || Context->LogicalWidth == 0 || Context->LogicalHeight == 0) return EFI_NOT_READY;

  Temporary = NULL;
  switch (BltOperation) {
    case EfiBltVideoFill:
      if (BltBuffer == NULL) return EFI_INVALID_PARAMETER;
      Status = TransformRectCW(Context, DestinationX, DestinationY, Width, Height, &PhysicalRect);
      if (EFI_ERROR(Status)) return Status;
      return Context->OriginalBlt(This, BltBuffer, EfiBltVideoFill, 0, 0, PhysicalRect.X, PhysicalRect.Y, PhysicalRect.Width, PhysicalRect.Height, 0);

    case EfiBltBufferToVideo:
      if (BltBuffer == NULL) return EFI_INVALID_PARAMETER;
      Status = TransformRectCW(Context, DestinationX, DestinationY, Width, Height, &PhysicalRect);
      if (EFI_ERROR(Status)) return Status;
      Status = CalculateBufferStride(SourceX, Width, Delta, &Stride);
      if (EFI_ERROR(Status)) return Status;
      if (!SafeMultiply(Width, Height, &PixelCount) || !SafeMultiply(PixelCount, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL), &BufferBytes)) return EFI_OUT_OF_RESOURCES;
      Temporary = AllocatePool(BufferBytes);
      if (Temporary == NULL) return EFI_OUT_OF_RESOURCES;
      LogicalToPhysicalBuffer(BltBuffer, Stride, SourceX, SourceY, Width, Height, Temporary);
      Status = Context->OriginalBlt(This, Temporary, EfiBltBufferToVideo, 0, 0, PhysicalRect.X, PhysicalRect.Y, PhysicalRect.Width, PhysicalRect.Height, PhysicalRect.Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      FreePool(Temporary);
      return Status;

    case EfiBltVideoToBltBuffer:
      if (BltBuffer == NULL) return EFI_INVALID_PARAMETER;
      Status = TransformRectCW(Context, SourceX, SourceY, Width, Height, &PhysicalRect);
      if (EFI_ERROR(Status)) return Status;
      Status = CalculateBufferStride(DestinationX, Width, Delta, &Stride);
      if (EFI_ERROR(Status)) return Status;
      if (!SafeMultiply(Width, Height, &PixelCount) || !SafeMultiply(PixelCount, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL), &BufferBytes)) return EFI_OUT_OF_RESOURCES;
      Temporary = AllocatePool(BufferBytes);
      if (Temporary == NULL) return EFI_OUT_OF_RESOURCES;
      Status = Context->OriginalBlt(This, Temporary, EfiBltVideoToBltBuffer, PhysicalRect.X, PhysicalRect.Y, 0, 0, PhysicalRect.Width, PhysicalRect.Height, PhysicalRect.Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
      if (!EFI_ERROR(Status)) {
        PhysicalToLogicalBuffer(Temporary, Width, Height, BltBuffer, Stride, DestinationX, DestinationY);
      }
      FreePool(Temporary);
      return Status;

    case EfiBltVideoToVideo:
    {
      GOP_RECT PhysicalSource;
      GOP_RECT PhysicalDestination;
      Status = TransformRectCW(Context, SourceX, SourceY, Width, Height, &PhysicalSource);
      if (EFI_ERROR(Status)) return Status;
      Status = TransformRectCW(Context, DestinationX, DestinationY, Width, Height, &PhysicalDestination);
      if (EFI_ERROR(Status)) return Status;
      return Context->OriginalBlt(This, NULL, EfiBltVideoToVideo, PhysicalSource.X, PhysicalSource.Y, PhysicalDestination.X, PhysicalDestination.Y, PhysicalSource.Width, PhysicalSource.Height, 0);
    }

    default:
      return EFI_INVALID_PARAMETER;
  }
}

STATIC EFI_STATUS
InstallHook (IN EFI_HANDLE Handle)
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  GOP_ROTATION_CONTEXT *Context;
  EFI_STATUS Status;
  if (Handle == NULL) return EFI_INVALID_PARAMETER;

  Gop = NULL;
  Status = gBS->HandleProtocol(Handle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&Gop);
  if (EFI_ERROR(Status)) return Status;
  if (Gop == NULL || Gop->Mode == NULL || Gop->Mode->Info == NULL || Gop->Blt == NULL || Gop->QueryMode == NULL || Gop->SetMode == NULL) return EFI_UNSUPPORTED;
  if (FindContext(Gop) != NULL) return EFI_ALREADY_STARTED;

  Context = AllocateContext();
  if (Context == NULL) return EFI_OUT_OF_RESOURCES;
  ZeroMem(Context, sizeof(*Context));

  Context->Gop = Gop;
  Context->OriginalBlt = Gop->Blt;
  Context->OriginalQueryMode = Gop->QueryMode;
  Context->OriginalSetMode = Gop->SetMode;
  Context->PhysicalInfo = AllocateCopyPool(sizeof(*Context->PhysicalInfo), Gop->Mode->Info);

  if (Context->PhysicalInfo == NULL) {
    ZeroMem(Context, sizeof(*Context));
    return EFI_OUT_OF_RESOURCES;
  }

  Context->PhysicalWidth = Context->PhysicalInfo->HorizontalResolution;
  Context->PhysicalHeight = Context->PhysicalInfo->VerticalResolution;
  if (Context->PhysicalWidth == 0 || Context->PhysicalHeight == 0) {
    FreePool(Context->PhysicalInfo);
    ZeroMem(Context, sizeof(*Context));
    return EFI_UNSUPPORTED;
  }

  Context->LogicalWidth = Context->PhysicalHeight;
  Context->LogicalHeight = Context->PhysicalWidth;
  Context->SavedPhysicalModeInfo = Gop->Mode->Info;
  Context->SavedPhysicalSizeOfInfo = Gop->Mode->SizeOfInfo;

  Status = UpdateLogicalModeInfo(Context);
  if (EFI_ERROR(Status)) {
    FreePool(Context->PhysicalInfo);
    if (Context->LogicalInfo != NULL) FreePool(Context->LogicalInfo);
    ZeroMem(Context, sizeof(*Context));
    return Status;
  }

  Context->Installed = TRUE;
  mContextCount++;
  Gop->Blt = HookBlt;
  Gop->QueryMode = HookQueryMode;
  Gop->SetMode = HookSetMode;

  DEBUG((DEBUG_INFO, "GopRotationHook: installed physical %ux%u, logical %ux%u\n", Context->PhysicalWidth, Context->PhysicalHeight, Context->LogicalWidth, Context->LogicalHeight));
  return EFI_SUCCESS;
}

STATIC VOID EFIAPI
GopNotify (IN EFI_EVENT Event, IN VOID *NotifyContext)
{
  EFI_STATUS Status;
  EFI_HANDLE Handle;
  UINTN BufferSize;
  (VOID)Event;
  (VOID)NotifyContext;

  if (mNotifyRegistration == NULL) return;

  for (;;) {
    Handle = NULL;
    BufferSize = sizeof(Handle);
    Status = gBS->LocateHandle(ByRegisterNotify, NULL, mNotifyRegistration, &BufferSize, &Handle);
    if (EFI_ERROR(Status)) break;
    Status = InstallHook(Handle);
    DEBUG((DEBUG_INFO, "GopRotationHook: notification install: %r\n", Status));
  }
}

EFI_STATUS EFIAPI
GopRotationHookEntryPoint (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;
  EFI_HANDLE *Handles;
  UINTN HandleCount, Index;
  (VOID)ImageHandle;
  (VOID)SystemTable;

  Handles = NULL;
  HandleCount = 0;
  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &HandleCount, &Handles);

  if (!EFI_ERROR(Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = InstallHook(Handles[Index]);
      DEBUG((DEBUG_INFO, "GopRotationHook: existing GOP %u: %r\n", Index, Status));
    }
    if (Handles != NULL) FreePool(Handles);
  } else if (Status != EFI_NOT_FOUND) {
    DEBUG((DEBUG_WARN, "GopRotationHook: LocateHandleBuffer failed: %r\n", Status));
  }

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, GopNotify, NULL, &mNotifyEvent);
  if (EFI_ERROR(Status)) return Status;

  Status = gBS->RegisterProtocolNotify(&gEfiGraphicsOutputProtocolGuid, mNotifyEvent, &mNotifyRegistration);
  if (EFI_ERROR(Status)) {
    gBS->CloseEvent(mNotifyEvent);
    mNotifyEvent = NULL;
    return Status;
  }

  DEBUG((DEBUG_INFO, "GopRotationHook: initialized\n"));
  return EFI_SUCCESS;
}