#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$ROOT/verify-bundle.sh"
"$ROOT/preflight.sh"
MODULE_DST="/kernel/drv/amd64/iwx"
FW_DIR="/kernel/firmware/iwx"
FW_DST="$FW_DIR/iwx-so-a0-hr-b0-77"
STATE="/var/adm/ax101-run106.state"
[ ! -e "$STATE" ] || {
    echo "ERROR: state file already exists; inspect it or run rollback.sh"
    exit 1
}
STAMP=$(date +%Y%m%d-%H%M%S)
BACKUP="/var/adm/ax101-run106-backup-$STAMP"
mkdir -p "$BACKUP" "$(dirname "$MODULE_DST")" "$FW_DIR"
MODULE_HAD_OLD=NO
FIRMWARE_HAD_OLD=NO
if [ -f "$MODULE_DST" ]; then
    cp -p "$MODULE_DST" "$BACKUP/iwx.previous"
    MODULE_HAD_OLD=YES
fi
if [ -f "$FW_DST" ]; then
    cp -p "$FW_DST" "$BACKUP/iwx-firmware.previous"
    FIRMWARE_HAD_OLD=YES
fi
{
    echo "BACKUP_DIR=$BACKUP"
    echo "MODULE_HAD_OLD=$MODULE_HAD_OLD"
    echo "FIRMWARE_HAD_OLD=$FIRMWARE_HAD_OLD"
    echo "REGISTERED=NO"
} > "$STATE"
chmod 0600 "$STATE"
cleanup_on_error() {
    rc=$?
    if [ "$rc" -ne 0 ]; then
        if [ "$MODULE_HAD_OLD" = YES ]; then
            cp -p "$BACKUP/iwx.previous" "$MODULE_DST"
        else
            rm -f "$MODULE_DST" "$MODULE_DST.new"
        fi
        if [ "$FIRMWARE_HAD_OLD" = YES ]; then
            cp -p "$BACKUP/iwx-firmware.previous" "$FW_DST"
        else
            rm -f "$FW_DST" "$FW_DST.new"
        fi
        rm -f "$STATE"
    fi
}
trap cleanup_on_error EXIT HUP INT TERM
cp "$ROOT/payload/iwx" "$MODULE_DST.new"
chmod 0644 "$MODULE_DST.new"
mv -f "$MODULE_DST.new" "$MODULE_DST"
cp "$ROOT/payload/iwx-so-a0-hr-b0-77" "$FW_DST.new"
chmod 0644 "$FW_DST.new"
mv -f "$FW_DST.new" "$FW_DST"
MODULE_SHA=$(digest -a sha256 "$MODULE_DST" | awk '{print $NF}')
FW_SHA=$(digest -a sha256 "$FW_DST" | awk '{print $NF}')
EXPECTED_MODULE=$(awk '$2 == "payload/iwx" {print $1}' "$ROOT/SHA256SUMS")
EXPECTED_FW=$(awk '$2 == "payload/iwx-so-a0-hr-b0-77" {print $1}' "$ROOT/SHA256SUMS")
[ "$MODULE_SHA" = "$EXPECTED_MODULE" ]
[ "$FW_SHA" = "$EXPECTED_FW" ]
trap - EXIT HUP INT TERM
echo "INSTALL_FILES=PASS"
echo "MODULE=$MODULE_DST"
echo "FIRMWARE=$FW_DST"
echo "BACKUP=$BACKUP"
echo "DRIVER_REGISTRATION=NOT_PERFORMED"
echo "MODULE_LOAD=NOT_PERFORMED"
