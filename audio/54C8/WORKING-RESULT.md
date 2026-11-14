# Chuwi MiniBook X N150 — working audio configuration

Status: **user-confirmed working on the target machine**, including a cold shutdown and subsequent power-on.

## Tested environment

- Hardware: Chuwi MiniBook X N150, Intel N150
- macOS: Monterey 12.7.6 (21H1320)
- HDA PCI controller: Intel `8086:54C8`
- Codec identified by the Linux diagnostic: Realtek ALC269VC
- Lilu: 1.7.3
- AppleALC: 1.9.9 experimental build with an Intel `0x54C8` controller entry
- Output: built-in speakers; user reports clean audio

## Working combination

The user tested these two files together and confirmed that the built-in speakers work, including after a full shutdown and power-on:

1. Experimental `AppleALC.kext` built from the repository's `audio/applealc-54c8-experiment` work.
2. OpenCore DeviceProperties entry at `PciRoot(0x0)/Pci(0x1f,0x3)`:
   - `layout-id` is **Data**, not Integer
   - bytes: `06 00 00 00` (layout 6)

The original configuration had `layout-id` represented as an Integer. The corrected plist stores it as Data. Do not change this type or value in the known-working setup.

## Artifact fingerprints

SHA-256 values for the tested artifacts distributed in this session:

- AppleALC executable (`AppleALC.kext/Contents/MacOS/AppleALC`): `4f8cea714b8c61014decd358c07e904cb1b8852779e24c93d1c9346eee0f014f`
- AppleALC bundle Info.plist: `e8735de5d1ae4baeb7c275dfbbd554d6cdc04d962c99e8e1fc4247f9bcff7984`
- Corrected config.plist: `4ff0427f25f78bbfc1f0b8374f9c288887daf6ea091e3e72df37989e1b30506c`

## Important qualification

The user confirmed that the **combination** works. This test does not isolate whether the success was caused by the 0x54C8 AppleALC controller entry, the corrected Data representation of layout-id, or their interaction. Keep both changes together as the known-good baseline.

The 0x54C8 controller entry was derived experimentally from the upstream 0x51C8 entry. Its successful operation is confirmed on this specific Chuwi machine; this is not a claim of universal compatibility with all 0x54C8 systems.

## Maintenance rule

- Preserve a complete backup of the working EFI.
- Do not update AppleALC or Lilu, alter the layout-id, or regenerate the audio kext automatically without first preserving this baseline.
- Use a separate test EFI for future experiments.
