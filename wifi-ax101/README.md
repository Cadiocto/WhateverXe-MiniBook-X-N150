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

This is stronger than simply adding a PCI alias: the source has a hardware-family/RF-specific AX101 selection path. The CI source gate checks the PCI mapping, HR1 constant and bit extraction, SO/HR1 table entry, live MAC/RF matching, firmware override, and requested API-77 filename against the pinned source. Run #24 evaluated 14 source-selection and allocator-bit checks against the pinned files; all 14 passed. The next gate revision adds five M_CANFAIL contract checks and a standalone flag-mapping test; those new checks remain pending until the next CI run. Note: the upstream compatibility header's comment for PCI macro `0x54f0` incorrectly labels it AX211; the target's subsystem `8086:0244` and RF ID `0x10c000` identify the user's AX101/HR1 configuration, so the comment is not used as evidence. The remaining uncertainty is runtime: actual MAC type/step, selected config, firmware acceptance/ALIVE and network operation on this Chuwi.

## Build and acceptance stages

1. Build the pinned illumos `iwx` module inside OpenIndiana 2026.10.
2. Run the AX101 source-selection gate and collect ELF metadata/symbol diagnostics. Strict ELF acceptance (undefined-symbol, relocation, and export policy) remains pending; current `elfdump` and `nm` output is diagnostic only.
3. Stage the API-77 HR firmware with its redistribution licence and checksum. No offline IPS repository or install kit has been produced yet.
4. Physical acceptance on this Chuwi: driver binds to `00:14.3`, reports AX101/HR1, loads matching firmware and receives ALIVE; then scan, WPA2 association, DHCP and traffic.

The CI build is not a hardware attach test. The driver must not be installed automatically or bound by default until firmware acceptance is proven.

## Scope

Wi-Fi only. No ROTFB, Xorg, Intel graphics, framebuffer, or kernel display changes in this branch.


## Current CI evidence

As of 2026-11-23, Runs #22 and #24 both completed the native OpenIndiana build and linked the same `iwx` artifact (SHA-256 `753223b974028399e327a57593b4efda62937a28a17d5f2afd70fe19cec009ad`). Run #24 also passed the pinned `run-mbuf-tests.sh` suite: 71 mbuf checks, 26 mlist checks, 23 cookie checks, and all 13 negative controls were caught. The existing net80211 mbuf harness and its negative control passed as well.

This is userland test plus translation-unit compile/link evidence. It does not prove module load, runtime attach, firmware ALIVE, scan, association, WPA2, DHCP, DNS, internet, or reconnect.

Run #24's baseline build emitted 559 warnings: 522 BSD/native `SIO*80211` macro redefinitions, 24 `m_free` redefinitions, 8 Ethernet macro redefinitions, one `roundup` redefinition, and four packed-member alignment warnings. The local `0004` overlay makes the intentional net80211 `m_free` replacement explicit; the next CI build fails if any `m_free` redefinition warning remains. The other warning groups remain open.

## WPA2 userspace contract

The GLDv3 adapter's WPA path expects the private `_wpa_psk` property to contain a precomputed 32-byte PMK as 64 hexadecimal characters, followed by a BSSID (or `-`) and SSID. It does not accept a passphrase directly. No userland PMK helper or secure SMF credential service is currently included in this repository, so WPA2-Personal is not yet an end-to-end usable path and persistent autoconnect is not implemented.


Run #25 (`45111068752`, job `137483300319`, commit `e47ab1086465e66fc16d2ed4b4fad5e9aab4124f`) also completed successfully. The log records 30 compiler invocations and the `ld -r -Breduce` link; the job completed with no compiler error diagnostics and uploaded artifact `14789557090` (811,640 bytes). The net80211 mbuf harness passed with `allocs==frees==20`; the iwx suites passed 71 mbuf, 26 mlist and 23 cookie checks, with the negative controls caught. The source gate passed its 14 existing checks. This remains compile/link and userland-test evidence, not module-load or physical-network evidence.

Run #25 emitted 535 compiler warning diagnostics: 522 BSD/native `SIO*80211` macro redefinitions, eight Ethernet macro redefinitions, one `roundup` redefinition and four packed-member alignment warnings. The 24 `m_free` redefinition warnings from Run #24 are absent after overlay 0004. The remaining warning groups are still open.
