#!/bin/sh
set -eu
[ "$(id -u)" -eq 0 ] || { echo "ERROR: run as root"; exit 1; }
STATE="/var/adm/ax101-run106.state"
[ -f "$STATE" ] || { echo "ERROR: no RUN106 state file at $STATE"; exit 1; }
. "$STATE"
case "$BACKUP_DIR" in
    /var/adm/ax101-run106-backup-*) ;;
    *) echo "ERROR: unsafe backup path in state file"; exit 1 ;;
esac
MODULE_DST="/kernel/drv/amd64/iwx"
FW_DST="/kernel/firmware/iwx/iwx-so-a0-hr-b0-77"
MODID=$(modinfo -c 2>/dev/null | awk '$3 == "iwx" && $4 == "LOADED/INSTALLED" {print $2; exit}')
if [ -n "$MODID" ]; then
    echo "Attempting controlled modunload of iwx module id $MODID"
    modunload -i "$MODID" || {
        echo "ERROR: modunload failed; stopping without deleting installed files"
        exit 2
    }
fi
if [ "$REGISTERED" = YES ]; then
    rem_drv iwx
fi
if [ "$MODULE_HAD_OLD" = YES ]; then
    [ -s "$BACKUP_DIR/iwx.previous" ] || {
        echo "ERROR: backup module missing"; exit 3;
    }
    cp -p "$BACKUP_DIR/iwx.previous" "$MODULE_DST"
else
    rm -f "$MODULE_DST"
fi
if [ "$FIRMWARE_HAD_OLD" = YES ]; then
    [ -s "$BACKUP_DIR/iwx-firmware.previous" ] || {
        echo "ERROR: firmware backup missing"; exit 3;
    }
    cp -p "$BACKUP_DIR/iwx-firmware.previous" "$FW_DST"
else
    rm -f "$FW_DST"
fi
rm -f "$STATE"
echo "ROLLBACK=PASS"
echo "Backup retained at $BACKUP_DIR"
