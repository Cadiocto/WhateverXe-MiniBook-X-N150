#!/usr/bin/bash
# Consolidated physical AX101 scan-discovery acceptance for RUN128.
# Run as root/pfexec after installing this bundle's iwx module.
set -Eeuo pipefail

OUT=${1:-/var/tmp/ax101-run128-physical-$(date -u +%Y%m%dT%H%M%SZ)}
BUNDLE_ROOT=$(cd -- "$(dirname -- "$0")" && pwd)
DTRACE_SCRIPT="$BUNDLE_ROOT/physical-ax101-scan.d"
mkdir -p "$OUT"

fail() {
    echo "FAIL: $*" | tee -a "$OUT/result.txt" >&2
    exit 1
}
capture() {
    label=$1
    shift
    echo "=== $label ===" | tee -a "$OUT/result.txt"
    set +e
    "$@" >"$OUT/$label.log" 2>&1
    rc=$?
    set -e
    echo "$label exit_code=$rc" | tee -a "$OUT/result.txt"
    return 0
}
section_has_rows() {
    file=$1
    begin=$2
    next=$3
    awk -v begin="$begin" -v end_marker="$next" '
        index($0, begin) { active = 1; next }
        active && index($0, end_marker) { exit }
        active && NF && $0 !~ /^No aggregations/ && $0 !~ /^dtrace:/ { rows++ }
        END { exit(rows > 0 ? 0 : 1) }
    ' "$file"
}

[ "$(id -u)" -eq 0 ] || fail "run as root or with pfexec"
[ "$(uname -v)" = "illumos-c893fd03ad" ] || fail "wrong kernel: $(uname -v)"
[ -s "$DTRACE_SCRIPT" ] || fail "bundle DTrace scan program is missing"
[ -e /dev/iwx0 ] || fail "/dev/iwx0 is missing; AX101 attach is not proven"
command -v dtrace >/dev/null 2>&1 || fail "dtrace is not installed"
command -v dladm >/dev/null 2>&1 || fail "dladm is not installed"
command -v prtconf >/dev/null 2>&1 || fail "prtconf is not installed"
command -v kstat >/dev/null 2>&1 || fail "kstat is not installed"

{
    echo "RUN=RUN128_PHYSICAL_AX101_SCAN"
    echo "UTC=$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    echo "KERNEL=$(uname -v)"
    echo "RELEASE=$(head -1 /etc/release)"
    echo "EXPECTED_PCI=8086:54F0"
    echo "EXPECTED_SUBSYSTEM=8086:0244"
    echo "EXPECTED_FUNCTION=00:14.3"
    echo "EXPECTED_DTRACE_SCAN_COMPLETE_PROBE=iwx_scan_complete_notification"
    echo "EXPECTED_SCAN_COMPLETE_OPCODE=IWX_SCAN_COMPLETE_UMAC"
    echo "EXPECTED_CONFIGURATION=iwx_cfg_so_a0_hr_b0"
    echo "EXPECTED_FIRMWARE=iwx-so-a0-hr-b0-77"
    echo "EXPECTED_FIRMWARE_API=77"
    echo "INTERFACE=iwx0"
    echo "DTRACE_PROGRAM=$DTRACE_SCRIPT"
} > "$OUT/identity.txt"

capture before-prtconf prtconf -pv
capture before-driver-list modinfo -c
capture before-phys dladm show-phys -m
capture before-links dladm show-link
capture before-wifi dladm show-wifi
capture before-kstat kstat -p
capture before-dmesg dmesg
capture before-ifconfig ifconfig -a

PCI_NODE=NO
PCI_SUBSYSTEM=NO
if grep -Eiq 'pci8086,54f0' "$OUT/before-prtconf.log"; then
    PCI_NODE=YES
fi
PCI_FUNCTION=NO
if grep -Eiq 'pci@14,3|pci@0,14,3' "$OUT/before-prtconf.log"; then
    PCI_FUNCTION=YES
fi
if grep -Eiq 'subsystem-vendor-id[[:space:]]*=[[:space:]]*(0x0*8086|0*32902)' "$OUT/before-prtconf.log" &&
   grep -Eiq 'subsystem-id[[:space:]]*=[[:space:]]*(0x0*244|0*580)' "$OUT/before-prtconf.log"; then
    PCI_SUBSYSTEM=YES
fi
IWX_LINK=NO
if grep -Eq '(^|[[:space:]])iwx0([[:space:]]|$)' "$OUT/before-phys.log" ||
   grep -Eq '(^|[[:space:]])iwx0([[:space:]]|$)' "$OUT/before-links.log"; then
    IWX_LINK=YES
fi
IWX_MOD=NO
if grep -Eq '(^|[[:space:]])iwx([[:space:]]|$)' "$OUT/before-driver-list.log"; then
    IWX_MOD=YES
fi
FIRMWARE_API77=NO
if grep -Eiq 'iwx-so-a0-hr-b0-77|firmware[^[:cntrl:]]*(API[ =:]*)?77|API[ =:]77' "$OUT/before-dmesg.log"; then
    FIRMWARE_API77=YES
    grep -Ei 'iwx-so-a0-hr-b0-77|firmware[^[:cntrl:]]*(API[ =:]*)?77|API[ =:]77' \
        "$OUT/before-dmesg.log" > "$OUT/firmware-api77-evidence.txt" || true
else
    echo "No explicit API-77 firmware filename/API line found in dmesg; inspect complete dmesg and driver logs." \
        > "$OUT/firmware-api77-evidence.txt"
fi
CONFIG_SO_HR1=NO
if grep -Fq 'iwx_cfg_so_a0_hr_b0' "$OUT/before-dmesg.log"; then
    CONFIG_SO_HR1=YES
fi
{
    echo "PCI_DEVICE_NODE=$PCI_NODE"
    echo "PCI_SUBSYSTEM=$PCI_SUBSYSTEM"
    echo "PCI_FUNCTION_00_14_3=$PCI_FUNCTION"
    echo "GLDV3_LINK_IWX0=$IWX_LINK"
    echo "IWX_MODULE_REGISTERED=$IWX_MOD"
    echo "FIRMWARE_API77_EVIDENCE=$FIRMWARE_API77"
    echo "CONFIG_SO_HR1_EVIDENCE=$CONFIG_SO_HR1"
} | tee -a "$OUT/result.txt"

# Capture the entire driver/firmware boot log for manual provenance review.
grep -Ei 'iwx|AX101|54f0|firmware|RF.?Kill|RF_KILL' "$OUT/before-dmesg.log" \
    > "$OUT/driver-boot-excerpt.txt" || true

# DTrace -c starts tracing and the real dladm scan as one command; no sleeps
# or probe-attach timing race are used. The iwx_scan_complete_notification
# SDT probe is emitted when IWX_SCAN_COMPLETE_UMAC is handled by the driver.
echo "SCAN_COMPLETE_PROBE=iwx_scan_complete_notification" | tee -a "$OUT/result.txt"
echo "SCAN_COMPLETE_OPCODE=IWX_SCAN_COMPLETE_UMAC" | tee -a "$OUT/result.txt"
set +e
dtrace -q -s "$DTRACE_SCRIPT" -c "/usr/sbin/dladm scan-wifi" \
    >"$OUT/scan-dtrace.log" 2>&1
DTRACE_SCAN_RC=$?
set -e
echo "DTRACE_SCAN_EXIT=$DTRACE_SCAN_RC" | tee -a "$OUT/result.txt"
[ "$DTRACE_SCAN_RC" -eq 0 ] || fail "DTrace scan command failed; see scan-dtrace.log"

capture after-wifi /usr/sbin/dladm show-wifi
capture after-phys /usr/sbin/dladm show-phys -m
capture after-ifconfig /sbin/ifconfig -a
capture after-kstat /usr/bin/kstat -p
capture after-dmesg /usr/bin/dmesg

# Preserve actual ESSID/BSSID rows. The raw dladm output remains authoritative
# if a platform release uses a different column layout.
python3 - "$OUT/after-wifi.log" "$OUT/essid-bssid-rows.txt" <<'PY'
import re
import sys
from pathlib import Path

source = Path(sys.argv[1]).read_text(errors="replace").splitlines()
target = Path(sys.argv[2])
header = None
for line in source:
    fields = line.split()
    if "ESSID" in fields and "BSSID" in fields:
        header = fields
        break
if header is None:
    target.write_text("ESSID_BSSID_HEADER=NOT_PARSED\n")
    print("ESSID_BSSID_HEADER=NOT_PARSED")
    raise SystemExit(0)
essid_col = header.index("ESSID")
bssid_col = header.index("BSSID")
mac = re.compile(r"(?i)(?:[0-9a-f]{2}:){5}[0-9a-f]{2}")
rows = []
for line in source:
    if not line.strip() or line.split() == header:
        continue
    fields = line.split()
    matches = list(mac.finditer(line))
    if not matches:
        continue
    bssid = matches[-1].group(0)
    before = line[:matches[-1].start()].split()
    if len(fields) <= bssid_col or len(fields) <= essid_col:
        continue
    essid = fields[essid_col]
    if essid in ("--", "-", "<unknown>", "ESSID"):
        continue
    rows.append((essid, bssid, line))
with target.open("w") as out:
    out.write("ESSID_BSSID_COUNT=%d\n" % len(rows))
    for essid, bssid, line in rows:
        out.write("ESSID=%s BSSID=%s ROW=%s\n" % (essid, bssid, line))
print("ESSID_BSSID_COUNT=%d" % len(rows))
for essid, bssid, line in rows:
    print("FOUND_ESSID_BSSID=%s %s" % (essid, bssid))
PY

SCAN_COMPLETE=NO
SCAN_RX_IDS=NO
BSS_TREE=NO
GLD_WL_SCAN=NO
GLD_ESS_SNAPSHOT=NO
RFKILL=NO
if section_has_rows "$OUT/scan-dtrace.log" \
    "=== Firmware IWX_SCAN_COMPLETE_UMAC notifications" "=== Firmware IWX_SCAN_ITERATION_COMPLETE_UMAC notifications"; then
    SCAN_COMPLETE=YES
fi
if section_has_rows "$OUT/scan-dtrace.log" \
    "=== RX HEADER:" "=== RX packet lengths"; then
    SCAN_RX_IDS=YES
fi
if section_has_rows "$OUT/scan-dtrace.log" \
    "=== BSS tree walked" "=== iwm_il_scan enter/exit"; then
    BSS_TREE=YES
fi
if section_has_rows "$OUT/scan-dtrace.log" \
    "=== GLDv3 WL_SCAN result ===" "=== GLDv3 ESS-list snapshot ==="; then
    GLD_WL_SCAN=YES
fi
if section_has_rows "$OUT/scan-dtrace.log" \
    "=== GLDv3 ESS-list snapshot ===" "=== END OF DTRACE PHYSICAL CAPTURE ==="; then
    GLD_ESS_SNAPSHOT=YES
fi
if section_has_rows "$OUT/scan-dtrace.log" \
    "=== RFKill raw CSR" "=== Valid RX MPDU lengths"; then
    RFKILL=YES
fi
ESSID_BSSID_COUNT=$(awk -F= '/^ESSID_BSSID_COUNT=/{print $2; exit}' "$OUT/essid-bssid-rows.txt")
[ -n "$ESSID_BSSID_COUNT" ] || ESSID_BSSID_COUNT=0
{
    echo "RX_NOTIFICATION_IDS_OBSERVED=$SCAN_RX_IDS"
    echo "IWX_SCAN_COMPLETE_UMAC_OBSERVED=$SCAN_COMPLETE"
    echo "BSS_TREE_SNAPSHOT_OBSERVED=$BSS_TREE"
    echo "RFKILL_EVIDENCE_OBSERVED=$RFKILL"
    echo "GLDV3_WL_SCAN_RESULT_OBSERVED=$GLD_WL_SCAN"
    echo "GLDV3_ESS_SNAPSHOT_OBSERVED=$GLD_ESS_SNAPSHOT"
    echo "IWX_SCAN_COMPLETE_UMAC_OBSERVED=$SCAN_COMPLETE"
    echo "ESSID_BSSID_COUNT=$ESSID_BSSID_COUNT"
    echo "PHYSICAL_ASSOCIATION=NOT_TESTED"
    echo "DHCP_DNS_INTERNET=NOT_TESTED"
    echo "EVIDENCE_DIRECTORY=$OUT"
} | tee -a "$OUT/result.txt"

# A physical discovery PASS requires the complete chain, not merely a
# successful scan command or a firmware command ACK.
if [ "$PCI_NODE" = YES ] && [ "$PCI_SUBSYSTEM" = YES ] &&
   [ "$PCI_FUNCTION" = YES ] && [ "$IWX_LINK" = YES ] && [ "$IWX_MOD" = YES ] &&
   [ "$FIRMWARE_API77" = YES ] && [ "$RFKILL" = YES ] &&
   [ "$SCAN_RX_IDS" = YES ] && [ "$SCAN_COMPLETE" = YES ] &&
   [ "$BSS_TREE" = YES ] && [ "$GLD_WL_SCAN" = YES ] &&
   [ "$GLD_ESS_SNAPSHOT" = YES ] && [ "$ESSID_BSSID_COUNT" -gt 0 ]; then
    echo "PHYSICAL_SCAN_DISCOVERY=PASS" | tee -a "$OUT/result.txt"
    echo "Review ESSID/BSSID rows and archive this evidence directory."
else
    echo "PHYSICAL_SCAN_DISCOVERY=NOT_PROVEN" | tee -a "$OUT/result.txt"
    echo "Physical acceptance is incomplete; inspect logs in $OUT" >&2
    exit 2
fi
