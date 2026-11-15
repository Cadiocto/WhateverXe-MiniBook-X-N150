# Changelog

## 0.1 — initial experimental implementation

- Added an EDK II X64 UEFI driver that wraps existing GOP interfaces.
- Added enumeration of GOP handles present at driver entry.
- Added GOP protocol notification for interfaces installed later.
- Added wrappers for Blt, QueryMode and SetMode.
- Added clockwise 90-degree software transformation for GOP BLT operations.
- Added checked rectangle and buffer-size calculations.
- Added an EDK II GitHub Actions build workflow.
