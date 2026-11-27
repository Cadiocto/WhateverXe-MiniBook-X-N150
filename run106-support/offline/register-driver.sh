#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$ROOT/preflight.sh"
[ "$(id -u)" -eq 0 ] || { echo "ERROR: run as root"; exit 1; }
[ -s /kernel/drv/amd64/iwx ] || {
    echo "ERROR: module is not installed; run install-manual.sh first"
    exit 1
}
[ -s /kernel/firmware/iwx/iwx-so-a0-hr-b0-77 ] || {
    echo "ERROR: API-77 firmware is not installed"
    exit 1
}
if grep -Eq '^iwx[[:space:]]' /etc/name_to_major 2>/dev/null; then
    echo "ERROR: iwx is already registered; refusing to replace existing registration"
    exit 1
fi
if ! prtconf -pv 2>/dev/null | grep -qi 'pci8086,54f0'; then
    echo "ERROR: AX101 PCI compatible string pci8086,54f0 not found; registration refused"
    exit 1
fi
add_drv -v -i '"pci8086,54f0"' iwx
grep -Eq '^iwx[[:space:]]+[0-9]+' /etc/name_to_major
STATE="/var/adm/ax101-run106.state"
if [ -f "$STATE" ]; then
    sed 's/^REGISTERED=.*/REGISTERED=YES/' "$STATE" > "$STATE.new"
    mv -f "$STATE.new" "$STATE"
fi
echo "DRIVER_REGISTRATION=PASS"
echo "MODULE_LOAD=NOT_PERFORMED"
echo "Next controlled step after reviewing dmesg: modload /kernel/drv/amd64/iwx"
