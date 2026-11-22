#!/usr/bin/env bash
# Static AX101/So/HR1 selection gate against the pinned OpenBSD iwx source.
set -euo pipefail
IWX=${1:?path to src/ports/wifi/iwx}
python3 - "$IWX/upstream/if_iwx.c" "$IWX/upstream/if_iwxvar.h" "$IWX/upstream/if_iwxreg.h" "$IWX/compat/dev/pci/pcidevs.h" <<'PY'
import re, sys
src, var, reg, pci = (open(p, encoding="utf-8").read() for p in sys.argv[1:])
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
}
failed = []
for label, ok in checks.items():
    print(("PASS" if ok else "FAIL") + "\t" + label)
    if not ok:
        failed.append(label)
if failed:
    raise SystemExit("AX101 source gate failed: " + "; ".join(failed))
print("AX101_SO_HR1_STATIC_SOURCE_GATE=PASS")
PY
