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
- The device-config table has generic SO/HR1 and SOF/HR1 entries explicitly labelled AX101.
- The attach path calls `iwx_find_device_cfg()`; when a match is found, it replaces the initial firmware name with the selected config's firmware name.
- The HR firmware API-77 name is `iwx-so-a0-hr-b0-77`. The matching binary exists in linux-firmware; its firmware version is `74.f92b5fed.0`.

This is stronger than simply adding a PCI alias: the source has a hardware-family/RF-specific AX101 selection path. The new CI source gate checks each of these facts against the pinned source. The remaining uncertainty is runtime: actual MAC type/step, selected config, firmware acceptance/ALIVE and network operation on this Chuwi.

## Build and acceptance stages

1. Build the pinned illumos `iwx` module inside OpenIndiana 2026.10.
2. Run the AX101 source-selection gate and verify the ELF module and symbol set.
3. Package the API-77 HR firmware with its redistribution licence.
4. Physical acceptance on this Chuwi: driver binds to `00:14.3`, reports AX101/HR1, loads matching firmware and receives ALIVE; then scan, WPA2 association, DHCP and traffic.

The CI build is not a hardware attach test. The driver must not be installed automatically or bound by default until firmware acceptance is proven.

## Scope

Wi-Fi only. No ROTFB, Xorg, Intel graphics, framebuffer, or kernel display changes in this branch.
