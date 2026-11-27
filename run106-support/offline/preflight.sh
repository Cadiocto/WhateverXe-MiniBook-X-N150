#!/bin/sh
set -eu
TARGET_KERNEL="illumos-c893fd03ad"
echo "TARGET_LAPTOP_KERNEL=$TARGET_KERNEL"
echo "OPENINDIANA_GUEST_KERNEL=$(uname -v)"
uname -a
uname -v
cat /etc/release
KERNEL_KIND=$(isainfo -k 2>/dev/null || true)
echo "KERNEL_KIND=$KERNEL_KIND"
[ "$(uname -v)" = "$TARGET_KERNEL" ] || {
    echo "PREFLIGHT=FAIL: kernel build mismatch"
    exit 20
}
[ "$KERNEL_KIND" = "amd64" ] || {
    echo "PREFLIGHT=FAIL: expected amd64 kernel modules"
    exit 21
}
[ "$(id -u)" -eq 0 ] || {
    echo "PREFLIGHT=FAIL: run as root"
    exit 22
}
echo "AX101_PCI_ID_EXPECTED=8086:54F0"
if prtconf -pv 2>/dev/null | grep -qi 'pci8086,54f0'; then
    echo "AX101_PCI_PRESENT=YES"
else
    echo "AX101_PCI_PRESENT=NOT_CONFIRMED"
    echo "WARNING: PCI compatible string was not found by this preflight probe"
fi
echo "PREFLIGHT=PASS"
