# Intel AX101 — OpenIndiana Hipster 2026.10 — RUN128

This offline bundle is the RUN128 native build candidate for the exact target kernel `illumos-c893fd03ad`. It is not compatible with a different `uname -v`.

## Hardware and firmware identity

- Intel AX101 PCI `8086:54F0`, subsystem `8086:0244`, function `00:14.3`
- MAC family SO, RF family HR1, configuration `iwx_cfg_so_a0_hr_b0`
- Firmware API 77, file `iwx-so-a0-hr-b0-77`
- Pinned Solnix and illumos-gate revisions, source commit and branch are recorded in `build-provenance.txt`
- The module in `payload/iwx` is built in this RUN128 guest run; it is not the older RUN106 module.

## Safe installation

Run from the extracted bundle directory as root or via `pfexec`:

1. `sh ./verify-bundle.sh`
2. `sh ./preflight.sh`
3. `sh ./install-manual.sh` — copies the module and firmware with backups; it does not register or load the driver.
4. Review `sh ./diagnose-ax101.sh`.
5. `sh ./register-driver.sh` — registers the PCI alias; it does not issue `modload`.

After reviewing the registration and kernel logs, load explicitly:

```sh
modload /kernel/drv/amd64/iwx
modinfo -c | grep iwx
dmesg | tail -n 100
```

## Consolidated physical scan acceptance

After installing and loading this bundle on the actual AX101 laptop, run:

```sh
pfexec ./physical-ax101-validation.sh
```

An alternate evidence directory can be supplied:

```sh
pfexec ./physical-ax101-validation.sh /var/tmp/ax101-run128-physical
```

The test records the exact kernel, PCI device and subsystem properties, driver registration and GLDv3 link, firmware/API-77 boot evidence, RFKill state, RX notification IDs, scan iteration/final-complete notifications, BSS tree activity and GLDv3 ESS snapshot. DTrace is started with the real `dladm scan-wifi` command, avoiding a sleep-based attach race. Raw `dladm show-wifi`, `ifconfig`, `kstat`, and `dmesg` output is preserved in the evidence directory.

It exits PASS only when the complete chain is observed and at least one non-placeholder ESSID/BSSID pair is reported. A scan command ACK or successful native build alone is not physical discovery evidence. If a firmware/API, PCI, scan-completion, BSS or GLDv3 observation is missing, the result remains NOT_PROVEN and the raw logs are retained.

WPA2 association, DHCP, DNS and Internet traffic are separate later acceptance gates. This bundle does not claim those results or pre-claim physical network discovery.

## Rollback

Run `sh ./rollback.sh` as root. It attempts a controlled module unload, removes the alias only if registered by this installer, and restores prior module and firmware files from the recorded backup. Keep the backup directory until validation is complete.

`connect-wifi.py` calculates the 32-byte WPA2 PMK from passphrase and SSID. It does not contain credentials or automatically change network configuration.
