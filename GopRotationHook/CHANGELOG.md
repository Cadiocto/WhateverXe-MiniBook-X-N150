# Changelog

## 0.1 — initial experimental implementation

- Added an EDK II X64 UEFI driver that wraps existing GOP interfaces.
- Added enumeration of GOP handles present at driver entry.
- Added GOP protocol notification for interfaces installed later.
- Added wrappers for Blt, QueryMode and SetMode.
- Added clockwise 90-degree software transformation for GOP BLT operations.
- Added checked rectangle and buffer-size calculations.
- Added an EDK II GitHub Actions build workflow.

## Hardware validation update

- Recorded a user-confirmed visual screen check on the actual CHUWI MiniBook X N150.
- Confirmed that the displayed screen image/orientation is correct on physical hardware.
- Clarified that this visual check does not constitute completion of the full GOP BLT, touch-input, boot-application or recovery test matrix.
