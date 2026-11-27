#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"
[ -s SHA256SUMS ] || { echo "ERROR: SHA256SUMS missing"; exit 1; }
fail=0
while IFS=' ' read -r expected rel; do
    [ -n "$expected" ] || continue
    case "$rel" in
        ""|/*|*..*) echo "ERROR: unsafe manifest path: $rel"; fail=1; continue ;;
    esac
    [ -f "$rel" ] || { echo "ERROR: missing $rel"; fail=1; continue; }
    actual=$(digest -a sha256 "$rel" | awk '{print $NF}')
    if [ "$actual" = "$expected" ]; then
        echo "PASS $rel"
    else
        echo "FAIL $rel expected=$expected actual=$actual"
        fail=1
    fi
done < SHA256SUMS
for required in payload/iwx payload/iwx-so-a0-hr-b0-77 LICENSE.iwlwifi_firmware preflight.sh install-manual.sh register-driver.sh diagnose-ax101.sh rollback.sh connect-wifi.py build-provenance.txt; do
    [ -f "$required" ] || { echo "ERROR: required file absent: $required"; fail=1; }
done
[ "$fail" -eq 0 ] || exit 1
echo "AX101_RUN106_BUNDLE_VERIFY=PASS"
