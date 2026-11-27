# Intel AX101 — OpenIndiana Hipster 2026.10 — RUN106

This bundle is intended only for the exact target kernel illumos-c893fd03ad. It is not compatible with RUN105's VM kernel illumos-336772c3cc. The OpenIndiana release label by itself is not used as a kernel compatibility check.

## Hardware and firmware identity

- Intel AX101 PCI 8086:54F0, subsystem 8086:0244, function 14,3
- MAC family SO, RF family HR1, configuration iwx_cfg_so_a0_hr_b0
- Firmware API 77, file iwx-so-a0-hr-b0-77
- Pinned Solnix and illumos-gate revisions are recorded in build-provenance.txt

## Safe installation sequence

Run these commands as root from the extracted bundle directory:

1. sh ./verify-bundle.sh
2. sh ./preflight.sh
3. sh ./install-manual.sh — copies the module and firmware with backups; it does not register or load the driver.
4. Review the output of sh ./diagnose-ax101.sh.
5. sh ./register-driver.sh — separately registers the PCI alias; it does not issue modload.

Only after reviewing the registration and kernel log, load explicitly:

    modload /kernel/drv/amd64/iwx
    modinfo -c | grep iwx
    dmesg | tail -n 100

For rollback, run sh ./rollback.sh as root. It attempts a controlled unload, removes the alias only if RUN106 registered it, and restores prior module and firmware files from the backup.

## Network bring-up

Module compilation and modload/modunload validation do not prove physical PCI attach, firmware ALIVE, scan, WPA2, DHCP, DNS, or Internet access on the Chuwi. These are separate hardware acceptance gates.

The connect-wifi.py helper follows the driver userspace PMK contract: it calculates the 32-byte WPA2 PMK from the passphrase and SSID. The bundle contains no Wi-Fi credentials and does not automatically change network configuration.

## Recovery

Do not delete /var/adm/ax101-run106-backup-* until the driver has been tested and accepted. If registration or module loading fails, save the diagnose-ax101.sh output and run rollback.sh.
