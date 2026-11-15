# Architecture notes

## Purpose

The target CHUWI MiniBook X N150 (CWI558) exposes a portrait-oriented physical GOP mode (reported as 1200 × 1920). The desired pre-OS presentation is landscape (1920 × 1200).

GopRotationHook does not reconfigure the Intel display engine. It wraps the UEFI Graphics Output Protocol and translates logical GOP operations into physical GOP operations with a clockwise 90-degree coordinate/pixel transform.

## Driver lifecycle

1. The entry point enumerates all handles currently exposing EFI_GRAPHICS_OUTPUT_PROTOCOL using LocateHandleBuffer(ByProtocol, ...).
2. Each supported GOP instance gets a context containing its original Blt, QueryMode and SetMode pointers and its physical mode information.
3. The driver installs wrappers by replacing those three function pointers on the GOP interface.
4. It registers RegisterProtocolNotify for GOP so later GOP installations can be wrapped too.
5. The notification callback drains handles registered under its notification key and calls the same install routine. Already-wrapped interfaces are rejected through context lookup.

The static context table currently supports up to eight GOP interfaces.

## Logical and physical mode

For a physical mode:

- physical width = 1200
- physical height = 1920

The logical mode exposed to callers is:

- logical width = physical height = 1920
- logical height = physical width = 1200

The logical mode information is a separately allocated copy. The original physical mode information and framebuffer metadata are retained so the original GOP methods can be called with the physical view restored temporarily.

## GOP methods wrapped

### QueryMode

Calls the original GOP QueryMode, then swaps horizontal and vertical resolution in the returned mode information and sets PixelsPerScanLine to the logical width.

### SetMode

Temporarily restores the physical mode-info pointer, delegates to the original SetMode, saves the newly selected physical mode and framebuffer metadata, then creates and publishes updated logical mode information.

### Blt

The wrapper handles the four standard BLT operations:

| Operation | Transform |
|---|---|
| EfiBltVideoFill | Transform destination rectangle and delegate fill |
| EfiBltBufferToVideo | Rotate source pixels into a temporary buffer, transform destination rectangle, delegate |
| EfiBltVideoToBltBuffer | Transform source rectangle, read into a temporary buffer, inverse-map pixels into caller buffer |
| EfiBltVideoToVideo | Transform source and destination rectangles, delegate copy |

Rectangle validation and checked arithmetic are used before calculating transformed coordinates and buffer sizes.

## Rotation mapping

For a logical rectangle with origin (x, y) and size (w, h), clockwise mapping into the physical surface is:

- physical x = physical width − (y + h)
- physical y = x
- physical width = h
- physical height = w

The pixel-buffer transform uses the corresponding coordinate mapping. This is a software transform in the BLT path, not a scanout-plane rotation.

## Important constraints

- A GOP client that writes directly to FrameBufferBase bypasses the wrapper.
- The driver does not currently wrap Simple Pointer or Absolute Pointer protocols; touch/mouse coordinates may not match the rotated display.
- Temporary BLT buffers add CPU and memory overhead.
- The current implementation has no complete driver-unload path to restore the original GOP function pointers.
- Firmware-specific GOP behavior, multiple GOP instances, mode changes and BLT edge cases require testing on physical hardware.

## Security and recovery

Test from a UEFI Shell or another recoverable boot path. Keep an unmodified firmware configuration and a known-good boot entry available. Do not flash a modified firmware image merely to test this driver.
