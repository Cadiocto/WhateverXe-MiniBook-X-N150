# Build and hardware test plan

## Build validation

The project CI workflow builds an X64 UEFI driver against EDK II edk2-stable202405.

Expected output:

```text
Build/OvmfX64/DEBUG_GCC5/X64/GopRotationHook.efi
```

A previous CI run (Build #13) completed successfully. Re-run the workflow after every source or build-system change; a historical successful build does not prove that the current branch head builds.

## Safe first test

1. Keep a known-good UEFI boot path and recovery USB available.
2. Copy GopRotationHook.efi to a FAT32 USB drive.
3. Start the UEFI Shell and run map -r.
4. Identify the USB filesystem mapping and load the driver:
   ```text
   Shell> load fsN:\\GopRotationHook.efi
   ```
5. Use a GOP-aware utility or test application to query the active mode.
6. Verify the logical mode is 1920 × 1200 and that the display output is visually landscape.
7. Reboot without loading the driver and confirm the firmware's original portrait mode remains available.

Do not assume that a successful load means every GOP client is transformed. Some applications may access the framebuffer directly rather than use GOP BLT.

## BLT test matrix

Test all operations with small rectangles first, then full-screen and edge-touching rectangles.

| Test | Expected result |
|---|---|
| VideoFill | Correctly rotated solid rectangle |
| BufferToVideo | Correct clockwise pixel orientation |
| VideoToBltBuffer | Correct inverse mapping into destination buffer |
| VideoToVideo | Correctly transformed source and destination regions |
| Zero width/height | No-op success |
| Out-of-range rectangle | Rejected without framebuffer corruption |
| Large rectangle / allocation pressure | Clean error, no hang |
| Repeated SetMode | Logical dimensions remain consistent |
| Multiple GOP handles | Each supported instance is wrapped once |

## Target-device acceptance

Record:

- Exact firmware version.
- Physical GOP mode before loading the driver.
- Logical GOP mode after loading the driver.
- Whether the UEFI Shell, boot manager and rEFInd all show landscape output.
- Whether any screen remains portrait because it uses direct framebuffer access.
- Whether touch coordinates remain aligned (they are not transformed by this driver today).
- Serial/debug output and any EFI error status.

## Not yet claimed

This document does not claim that the driver has passed the above physical-device test matrix. CI compilation and hardware validation are separate milestones.
