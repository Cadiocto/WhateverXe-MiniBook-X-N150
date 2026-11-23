#!/usr/bin/env bash
# Static AX101/So/HR1 selection gate against the pinned OpenBSD iwx source.
set -euo pipefail
IWX=${1:?path to src/ports/wifi/iwx}
WIFI_DIR=$(cd "$IWX/.." && pwd)
python3 - "$IWX/upstream/if_iwx.c" "$IWX/upstream/if_iwxvar.h" "$IWX/upstream/if_iwxreg.h" "$IWX/compat/dev/pci/pcidevs.h" "$IWX/compat/sys/iwx_kernel.h" "$IWX/compat/sys/mbuf.h" "$WIFI_DIR/net80211/n80211_kcompat.c" "$WIFI_DIR/net80211/compat/sys/n80211_alloc.h" <<'PY'
import re, sys
if len(sys.argv) != 9:
    raise SystemExit(f"expected 8 source paths, got {len(sys.argv) - 1}")
src, var, reg, pci, kernel, mbuf, kcompat, alloc = (open(p, encoding="utf-8").read() for p in sys.argv[1:])

def canfail_sites_null_checked(source):
    sites = list(re.finditer(r"malloc\([^;]*\bM_CANFAIL\b[^;]*\);", source, re.S))
    if len(sites) != 5:
        return False
    for site in sites:
        prefix = source[max(0, site.start() - 160):site.start()]
        lhs = re.search(r"([A-Za-z_]\w*(?:(?:->|\.)\w+|\[[^\]]+\])*)\s*=\s*$", prefix)
        if lhs is None:
            return False
        name = lhs.group(1)
        tail = source[site.end():site.end() + 200]
        if not re.search(r"if\s*\(\s*" + re.escape(name) + r"\s*==\s*NULL\s*\)", tail):
            return False
    return True
checks = {
    "PCI device 54f0 maps to the device-table symbol used by iwx":
        re.search(r"PCI_PRODUCT_INTEL_WL_22500_16\s+0x54f0", pci, re.I)
        and "PCI_PRODUCT_INTEL_WL_22500_16" in src,
    "RF type HR1 is 0x10c":
        re.search(r"IWX_CFG_RF_TYPE_HR1\s+0x10c", var + reg, re.I),
    "RFID type extraction uses bits 23:12":
        re.search(r"IWX_CSR_HW_RFID_TYPE\(_val\).*?0x0FFF000\)\s*>>\s*12", reg),
    "So MAC + HR1 has an AX101 configuration entry":
        re.search(r"IWX_CFG_MAC_TYPE_SO,\s*IWX_CFG_ANY,\s*IWX_CFG_RF_TYPE_HR1,.*?iwx_cfg_so_a0_hr_b0\),\s*/\*\s*ax101\s*\*/", src, re.S | re.I),
    "device-config matcher checks the actual MAC type":
        re.search(r"dev_info->mac_type\s*!=.*?mac_type", src, re.S),
    "device-config matcher checks the actual RF type":
        re.search(r"dev_info->rf_type\s*!=.*?rf_type", src, re.S),
    "attach path replaces default firmware with matched device config":
        re.search(r"cfg\s*=\s*iwx_find_device_cfg\(sc\);\s*if\s*\(cfg\)\s*\{\s*sc->sc_fwname\s*=\s*cfg->fw_name;", src, re.S),
    "HR firmware API-77 name is present":
        "iwx-so-a0-hr-b0-77" in var + src,
    "allocator M_WAITOK is the OpenBSD 0x0001 bit":
        re.search(r"^\s*#define\s+M_WAITOK\s+0x0001\b", kernel, re.M),
    "allocator M_NOWAIT is the OpenBSD 0x0002 bit":
        re.search(r"^\s*#define\s+M_NOWAIT\s+0x0002\b", kernel, re.M),
    "allocator M_CANFAIL is the OpenBSD 0x0004 bit":
        re.search(r"^\s*#define\s+M_CANFAIL\s+0x0004\b", kernel, re.M),
    "mbuf M_DONTWAIT aliases M_NOWAIT":
        re.search(r"^\s*#define\s+M_DONTWAIT\s+M_NOWAIT\b", mbuf, re.M),
    "mbuf M_WAIT aliases M_WAITOK":
        re.search(r"^\s*#define\s+M_WAIT\s+M_WAITOK\b", mbuf, re.M),
    "mbuf shim does not redefine allocator flag macros":
        not re.search(r"^\s*#define\s+M_(?:WAITOK|NOWAIT|CANFAIL|ZERO|DEVBUF)\b", mbuf, re.M),
    "iwx_bsd_malloc uses the tested allocator flag helper":
        re.search(r"int\s+km\s*=\s*iwx_bsd_kmem_flags\(flags\)\s*;", kcompat),
    "allocator helper maps M_NOWAIT to KM_NOSLEEP":
        re.search(r"if\s*\(\s*\(flags\s*&\s*M_NOWAIT\)\s*!=\s*0\s*\)\s*return\s*\(KM_NOSLEEP\);", alloc),
    "allocator helper maps M_CANFAIL to KM_NOSLEEP":
        re.search(r"if\s*\(\s*\(flags\s*&\s*M_CANFAIL\)\s*!=\s*0\s*\)\s*return\s*\(KM_NOSLEEP\);", alloc),
    "allocator helper defaults to KM_SLEEP":
        re.search(r"return\s*\(KM_SLEEP\);", alloc),
    "all five active M_CANFAIL malloc sites check for NULL":
        canfail_sites_null_checked(src),
}
failed = []
for label, ok in checks.items():
    print(("PASS" if ok else "FAIL") + "\t" + label)
    if not ok:
        failed.append(label)
if failed:
    raise SystemExit("AX101 source gate failed: " + "; ".join(failed))
print("AX101_SO_HR1_STATIC_SOURCE_GATE=PASS")
print("AX101_ALLOCATOR_FLAGS_STATIC_GATE=PASS")
PY
