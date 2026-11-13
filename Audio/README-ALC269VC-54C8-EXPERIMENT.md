# CHUWI MiniBook X N150 — audio controller experiment

Status: source-level experiment only; not a compiled kext and not validated on hardware.

## Hardware evidence
- Intel HDA PCI controller: 8086:54C8
- Realtek codec reported by Linux: 10EC:0269 (ALC269VC), revision 0x100203
- Linux codec nodes: 0x14 speaker, 0x15 headphone, 0x12 internal mic, 0x18 external mic
- macOS target: Monterey 12.7.6

## AppleALC controller-table candidate
Add this dict as a new element of the root array in Resources/Controllers.plist:

<dict>
  <key>Device</key>
  <integer>21704</integer>
  <key>Name</key>
  <string>Intel Alder Lake-N / Twin Lake HDA Controller (0x54C8) — experimental</string>
  <key>Patches</key>
  <array>
    <dict>
      <key>Count</key>
      <integer>1</integer>
      <key>Find</key>
      <data>DgAAvgIAAABIid8=</data>
      <key>MinKernel</key>
      <integer>19</integer>
      <key>Name</key>
      <string>AppleHDAController</string>
      <key>Replace</key>
      <data>DgAAuEijAADrBpA=</data>
    </dict>
  </array>
  <key>Vendor</key>
  <string>Intel</string>
</dict>

The Find/Replace pair is copied from the existing upstream AppleALC 0x51C8 controller entry. The Find bytes were observed once in the user's Monterey AppleHDAController binary at offset 0x21D1:
- Find: 0e 00 00 be 02 00 00 00 48 89 df
- Replace: 0e 00 00 b8 48 a3 00 00 eb 06 90

## Critical caveat
This is only a candidate. A matching byte signature does not establish that the replacement is semantically correct for PCI ID 0x54C8. The existing AppleALC 0x51C8 patch is not proof of compatibility with Alder Lake-N / Twin Lake. Do not install this as a release build.

## Test protocol
1. Build AppleALC from source with this entry, against the Monterey-compatible source/toolchain.
2. Use a separate OpenCore EFI copy; preserve the working EFI and graphics kexts.
3. Enable AppleALC debug logging (-alcdbg) for the test.
4. Verify that AppleALC detects HDEF and logs controller 8086:54C8 and that the controller patch is applied.
5. Check IORegistry for AppleHDAController, IOHDACodecDevice, and audio engines.
6. If controller service still does not instantiate, stop changing codec layout IDs and investigate HDEF/provider matching and AppleHDAController start path.

No changes to WhateverXe graphics components are part of this experiment.
