# Intel AX101 Wi-Fi port for OpenIndiana

Target hardware measured on the Chuwi MiniBook X N150:

- PCI function: `00:14.3`
- Vendor/device: `8086:54f0`
- Subsystem: `8086:0244`
- PCI revision: `0x370`
- RF ID: `0x10c000`; the iwx register definition extracts bits 23:12, yielding RF type `0x10c` = `HR1`.

## Driver source selected

Use the existing illumos port of OpenBSD `iwx(4)` from:

- Repository: https://github.com/atiq-cs/solnix
- Pinned source revision: `1cee7021091f3df92b7816ece2902ff59b37f1be`
- Driver source: `src/ports/wifi/iwx/`
- Native illumos build: `Makefile.illumos`
- OpenBSD net80211 compatibility layer: `src/ports/wifi/net80211/`
- DDI/GLDv3 glue and DMA/STREAMS adapters are part of that port.

This is a much better starting point than writing a Wi-Fi MAC/firmware driver from zero. The upstream port's status records a complete module build and a hardware-tested WPA2 association on a different Intel AX211 system. That proves the port architecture can work on illumos; it does not by itself prove AX101 operation.

## AX101 source-path result

The pinned OpenBSD source already contains the necessary AX101-specific ingredients:

- PCI ID `0x54f0` is in the `iwx` device table.
- `IWX_CFG_RF_TYPE_HR1` is `0x10c`.
- The register macro extracts RF type from bits 23:12, so the reported `0x10c000` becomes `0x10c` (HR1).
- The device-config table has an SO/HR1 entry explicitly labelled AX101. The matching logic compares the live MAC type and RF type; it is not enough for the source merely to contain an AX101 comment.
- The attach path calls `iwx_find_device_cfg()`; when a match is found, it replaces the initial firmware name with the selected config's firmware name.
- The pinned OpenBSD source names the SO/HR1 firmware API-77 image `iwx-so-a0-hr-b0-77`; the build workflow stages that exact candidate because it is what this source revision requests. This is not proof that API 77 is accepted by the Chuwi's silicon. Other current AX101 implementations request API 89 (`iwlwifi-so-a0-hr-b0-89`), so firmware acceptance/ALIVE remains a mandatory hardware gate. Do not substitute API 89 into this OpenBSD driver without checking its API parser and command/notification compatibility.

This is stronger than simply adding a PCI alias: the source has a hardware-family/RF-specific AX101 selection path. The CI source gate checks the PCI mapping, HR1 constant and bit extraction, SO/HR1 table entry, live MAC/RF matching, firmware override, and requested API-77 filename against the pinned source. All eight checks were independently evaluated against the pinned files and passed. Note: the upstream compatibility header's comment for PCI macro `0x54f0` incorrectly labels it AX211; the target's subsystem `8086:0244` and RF ID `0x10c000` identify the user's AX101/HR1 configuration, so the comment is not used as evidence. The remaining uncertainty is runtime: actual MAC type/step, selected config, firmware acceptance/ALIVE and network operation on this Chuwi.

## Build and acceptance stages

1. Build the pinned illumos `iwx` module inside OpenIndiana 2026.10.
2. Run the AX101 source-selection gate and verify the ELF module and symbol set.
3. Package the API-77 HR firmware with its redistribution licence.
4. Physical acceptance on this Chuwi: driver binds to `00:14.3`, reports AX101/HR1, loads matching firmware and receives ALIVE; then scan, WPA2 association, DHCP and traffic.

The CI build is not a hardware attach test. The driver must not be installed automatically or bound by default until firmware acceptance is proven.

## Scope

Wi-Fi only. No ROTFB, Xorg, Intel graphics, framebuffer, or kernel display changes in this branch.
