# GopRotationHook

<p align="center">
  <strong>UEFI GOP rotation hook for portrait-first Intel firmware</strong><br>
  A small EDK II UEFI driver that exposes a landscape logical GOP view over a portrait physical framebuffer.
</p>

<p align="center">
  <a href="https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/workflows/build-gop-rotation-hook.yml">
    <img src="https://github.com/Cadiocto/WhateverXe-MiniBook-X-N150/actions/workflows/build-gop-rotation-hook.yml/badge.svg?branch=gop-rotation-hook" alt="Build GopRotationHook">
  </a>
  <img src="https://img.shields.io/badge/EDK%20II-X64-blue" alt="EDK II X64">
  <img src="https://img.shields.io/badge/status-experimental-orange" alt="Experimental">
</p>

**Target platform:** CHUWI MiniBook X N150 (CWI558), Intel Alder Lake-N / N150  
**Target orientation:** physical GOP 1200 × 1920 portrait → logical GOP 1920 × 1200 landscape  
**Implementation:** clockwise 90° software transform in the GOP BLT path

> [!WARNING]
> Experimental firmware-level software hook. It changes GOP protocol function pointers in place. Test from a UEFI Shell or a recoverable boot setup first. Do not flash modified firmware as the first test.

## What it does

GopRotationHook is an EDK II UEFI_DRIVER that wraps an existing EFI_GRAPHICS_OUTPUT_PROTOCOL (GOP) instance and presents a rotated logical mode to UEFI applications.

- Enumerates GOP handles that already exist when the driver starts.
- Registers a protocol notification for GOP instances installed later.
- Wraps Blt, QueryMode and SetMode, preserving the original function pointers.
- Swaps the reported horizontal and vertical resolutions for the logical landscape view.
- Transforms BLT rectangles and pixel buffers for clockwise 90° rotation.
- Delegates the actual physical framebuffer operations to the original GOP implementation.

## What it does not do

- It is **not** an Intel GPU driver and does not program the Display Engine, planes, pipes or scanout registers.
- It does **not** provide hardware-accelerated rotation. Pixel transformation is performed by the CPU in the GOP BLT path.
- It does not change panel timings or the physical orientation of the panel.
- It does not currently rotate UEFI pointer/touch coordinates (Simple Pointer / Absolute Pointer).
- It does not guarantee that every boot application uses GOP BLT; applications that directly access the framebuffer may bypass the transform.

## Repository layout

```text
GopRotationHook/
├── GopRotationHook.c
├── GopRotationHook.inf
├── README.md
├── CHANGELOG.md
└── docs/
    ├── ARCHITECTURE.md
    └── TESTING.md
```

## Build

The GitHub Actions workflow builds against EDK II stable tag edk2-stable202405 for X64.

1. Open the repository's **Actions** tab.
2. Select **Build GopRotationHook**.
3. Choose **Run workflow** on branch gop-rotation-hook.
4. Download the GopRotationHook-efi artifact from the completed run.

The workflow checks out EDK II, prepares BaseTools and required submodules, adds GopRotationHook/GopRotationHook.inf to an OVMF X64 DSC, and builds the EFI module.

### Local build outline

Use an EDK II workspace with the required submodules initialized:

```bash
git clone https://github.com/tianocore/edk2.git
cd edk2
git checkout edk2-stable202405
git submodule update --init
make -C BaseTools
. ./edksetup.sh
```

Copy this repository's GopRotationHook/ directory into the EDK II workspace, add the INF to an OVMF X64 DSC [Components] section, then build:

```bash
build -a X64 -t GCC5 \
  -p OvmfPkg/OvmfPkgX64-GopRotationHook.dsc \
  -m GopRotationHook/GopRotationHook.inf \
  -b DEBUG
```

Expected output:

```text
Build/OvmfX64/DEBUG_GCC5/X64/GopRotationHook.efi
```

## Initial testing

For a first test, load the driver from a UEFI Shell on a machine where recovery is straightforward:

```text
Shell> load fs0:\\GopRotationHook.efi
```

The actual filesystem mapping may differ; locate the USB/device with map -r and use the correct fsN: mapping.

Then inspect the GOP mode reported by a UEFI application or shell utility. On the target device, the intended logical mode is **1920 × 1200** while the physical GOP mode is expected to remain **1200 × 1920**.

See [docs/TESTING.md](docs/TESTING.md) before testing on the laptop.

## Current status

| Area | Status |
|---|---|
| EDK II X64 module | Implemented |
| Existing GOP handle enumeration | Implemented |
| Notification for newly installed GOP | Implemented |
| GOP BLT clockwise transform | Implemented |
| QueryMode / SetMode wrappers | Implemented |
| CI compilation | Build #13 previously succeeded |
| CHUWI MiniBook X N150 physical-screen check | **Passed — user-confirmed on-device visual check; screen orientation/output is correct** |
| Full GOP BLT test matrix on hardware | Not yet confirmed |
| Pointer/touch coordinate rotation | Not implemented |
| Hardware display-plane rotation | Out of scope |

### Hardware validation note

The user confirmed that the screen was checked on the **actual CHUWI MiniBook X N150**, not in a virtual machine, and that the displayed image/orientation is correct. This records a successful physical-screen visual check only; it does not claim that every GOP BLT operation, touch input path, boot application or recovery/unload scenario has been validated.

## Known implementation limits

- The context table is statically limited to 8 GOP instances.
- The driver replaces protocol method pointers in place; it does not currently provide a complete unload path that restores every original pointer.
- BLT rotation allocates temporary buffers, so large updates may be slow and can fail with EFI_OUT_OF_RESOURCES.
- Direct framebuffer writes by boot applications are outside the BLT wrapper and will not be transformed.
- Multiple GOP handles and repeated SetMode calls require broader hardware testing.

## Contributing

Please include the following with bug reports:

- Firmware version and device model.
- GOP physical mode and expected logical mode.
- Which BLT operation was used (VideoFill, BufferToVideo, VideoToBltBuffer or VideoToVideo).
- Serial/debug log and whether the issue occurs before or after SetMode.

See [the architecture notes](docs/ARCHITECTURE.md) and [the test plan](docs/TESTING.md).

## License

No license has been selected for this project yet. Until a license is added, do not assume that the source is available for redistribution or modification under an open-source license.
