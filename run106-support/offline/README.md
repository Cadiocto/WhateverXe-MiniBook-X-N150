# Intel AX101 — OpenIndiana Hipster 2026.10 — RUN107 SCANFIX

This offline bundle contains a new native build of the scan-fix source candidate for the exact target kernel illumos-c893fd03ad. It is not compatible with RUN105's VM kernel illumos-336772c3cc or any kernel with a different uname -v.

## Hardware and firmware identity

- Intel AX101 PCI 8086:54F0, subsystem 8086:0244, function 14,3
- MAC family SO, RF family HR1, configuration iwx_cfg_so_a0_hr_b0
- Firmware API 77, file iwx-so-a0-hr-b0-77
- Pinned Solnix and illumos-gate revisions are recorded in build-provenance.txt
- The module was freshly compiled from the scan-fix source in this run; the old RUN106 module is not reused as the payload.

## Safe installation sequence

Run as root from the extracted bundle directory:

1. sh ./verify-bundle.sh
2. sh ./preflight.sh
3. sh ./install-manual.sh — copies the module and firmware with backups; it does not register or load the driver.
4. Review sh ./diagnose-ax101.sh.
5. sh ./register-driver.sh — registers the PCI alias; it does not issue modload.

Only after reviewing registration and kernel logs, load explicitly:

    modload /kernel/drv/amd64/iwx
    modinfo -c | grep iwx
    dmesg | tail -n 100

## One consolidated physical scan test

The bundle includes the reviewed source patch, deterministic regression tests, DTrace script, and one-shot physical scan runner under source/scanfix-implementation/.

After the driver is loaded on the target laptop, run exactly one consolidated capture as root:

    sh ./source/scanfix-implementation/tests/run-ax101-scan-validation.sh

The test checks the exact kernel, runs DTrace concurrently with a single dladm scan-wifi, then captures dladm show-wifi, interface flags, kstats and dmesg. It distinguishes scan command ACK from actual scan-complete notification, MPDU/beacon/probe reception, BSS tree population and GLDv3 ESS snapshot publication.

**Physical scan acceptance requires non-placeholder ESSID rows in dladm show-wifi and consistent beacon/probe/BSS evidence.** A clean native compile, firmware command ACK or zero exit code from dladm scan-wifi is not sufficient. This source build does not claim that physical AP discovery has already been verified.

Association, WPA2 handshake, DHCP, DNS and Internet traffic remain separate acceptance gates and are not claimed here.

## Rollback

Run sh ./rollback.sh as root. It attempts a controlled module unload, removes the alias only if registered by this installer, and restores prior module and firmware files from the backup. Do not delete /var/adm/ax101-run106-backup-* until validation is complete.

The connect-wifi.py helper calculates the 32-byte WPA2 PMK from passphrase and SSID. It does not contain credentials or automatically change network configuration.
