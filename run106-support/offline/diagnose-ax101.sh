#!/bin/sh
set +e
echo "=== KERNEL ==="
uname -a
uname -v
cat /etc/release
isainfo -kv
echo "=== PCI / DEVICE TREE ==="
prtconf -pv 2>&1 | grep -i -C 8 -E 'pci8086,54f0|vendor-id|device-id|8086|54f0|0244'
echo "=== DRIVER / MODULE ==="
grep -E '^iwx[[:space:]]' /etc/name_to_major /etc/driver_aliases 2>&1
modinfo -c 2>&1 | grep -i -C 2 iwx
echo "=== FIRMWARE ==="
ls -l /kernel/firmware/iwx/iwx-so-a0-hr-b0-77
digest -a sha256 /kernel/firmware/iwx/iwx-so-a0-hr-b0-77
echo "=== NETWORK LINKS ==="
dladm show-phys -m 2>&1
dladm show-link 2>&1
ipadm show-addr 2>&1
echo "=== RECENT KERNEL LOG ==="
dmesg | tail -n 300 | grep -i -E 'iwx|iwlwifi|firmware|pci|net80211|mac' | tail -n 200
