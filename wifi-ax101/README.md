# Intel AX101 Wi-Fi port for OpenIndiana

Target hardware measured on the Chuwi MiniBook X N150:

- PCI function: `00:14.3`
- Vendor/device: `8086:54f0`
- Subsystem: `8086:0244`
- Revision: `0x370`
- RF ID from the matching Linux report: `0x10c000`
- OpenIndiana currently sees the device in PCI enumeration, but no Wi-Fi driver is bound and `dladm show-phys` has no wireless interface.

## Driver source selected

Use the existing illumos port of OpenBSD `iwx(4)` from:

- Repository: https://github.com/atiq-cs/solnix
- Pinned source revision: `1cee7021091f3df92b7816ece2902ff59b37f1be`
- Driver source: `src/ports/wifi/iwx/`
- Native illumos build: `Makefile.illumos`
- OpenBSD net80211 compatibility layer: `src/ports/wifi/net80211/`
- DDI/GLDv3 glue and DMA/STREAMS adapters are part of that port.

This is a much better starting point than writing a Wi-Fi MAC/firmware driver from zero. The upstream port's status records a complete module build and a hardware-tested WPA2 association on a different Intel AX211 system. That is evidence for the port's architecture, NOT yet proof for this AX101.

## AX101-specific compatibility gate

Do not treat PCI ID matching as proof of hardware support.

The port's PCI table includes `8086:54f0`, but its older firmware selection path groups that ID with AX211/AX210 and defaults to the SO/GF firmware family. Linux identifies this exact `54f0/0244` device as AX101 and uses the SO/HR firmware family (`iwlwifi-so-a0-hr-b0-*.ucode`). The driver also contains an HR1 RF configuration path, so the next engineering task is to prove that the port's runtime CRF/RF detection selects that HR1 configuration on this machine and requests the matching HR firmware/PNVM. Do not install the GF firmware on the assumption that the PCI ID alone identifies the radio.

The Linux report for this exact subsystem reports `rev=0x370`, `rfid=0x10c000`; a later report shows the device detected as AX101. These values must be used as the AX101 fixture in source-level tests.

## Build and acceptance stages

1. Build the pinned illumos `iwx` module inside OpenIndiana 2026.10.
2. Verify the resulting ELF module, module entry points, undefined-symbol set and exact PCI alias.
3. Audit firmware selection for `54f0/0244` + HR1 and identify a firmware API revision actually accepted by this OpenBSD driver.
4. Only after stages 1–3 pass, assemble an installable module + firmware package.
5. Physical acceptance on this Chuwi: driver binds to `00:14.3`, reports AX101/HR1, loads matching firmware and receives ALIVE; then scan, WPA2 association, DHCP and traffic.

The CI build is not a hardware attach test. The driver must not be installed automatically or bound by default until the AX101 firmware-selection gate is passed.

## Scope

Wi-Fi only. No ROTFB, Xorg, Intel graphics, framebuffer, or kernel display changes in this branch.
