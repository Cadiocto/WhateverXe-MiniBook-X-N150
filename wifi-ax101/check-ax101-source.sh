#!/usr/bin/env bash
# Static AX101 selection gate against the pinned OpenBSD iwx source.
set -euo pipefail
IWX=${1:?path to src/ports/wifi/iwx}
python3 - "$IWX/upstream/if_iwx.c" "$IWX/upstream/if_iwxvar.h" "$IWX/upstream/if_iwxreg.h" "$IWX/compat/dev/pci/pcidevs.h" <<'PY'
import re, sys
src, var, reg, pci = (open(p, encoding="utf-8").read() for p in sys.argv[1:])
checks = {
    "PCI 54f0 is present in the port device table":
        re.search(r"PCI_PRODUCT_INTEL_WL_22500_16\s+0x54f0", pci, re.I),
    "RF type HR1 is 0x10c":
        re.search(r"IWX_CFG_RF_TYPE_HR1\s+0x10c", var, re.I),
    "RFID type extraction uses bits 23:12":
        re.search(r"IWX_CSR_HW_RFID_TYPE\(_val\).*?0x0FFF000\)\s*>>\s*12", reg),
    "SO + HR1 generic device config is explicitly AX101":
        re.search(r"IWX_CFG_MAC_TYPE_SO,\s*IWX_CFG_ANY,\s*IWX_CFG_RF_TYPE_HR1,.*?iwx_cfg_so_a0_hr_b0\),\s*/\*\s*ax101\s*\*/", src, re.S | re.I),
    "SOF + HR1 generic device config is explicitly AX101":
        re.search(r"IWX_CFG_MAC_TYPE_SOF,\s*IWX_CFG_ANY,\s*IWX_CFG_RF_TYPE_HR1,.*?iwx_cfg_so_a0_hr_b0\),\s*/\*\s*AX101\s*\*/", src, re.S | re.I),
    "attach path calls device-config override":
        re.search(r"cfg\s*=\s*iwx_find_device_cfg\(sc\);\s*if\s*\(cfg\)\s*\{\s*sc->sc_fwname\s*=\s*cfg->fw_name;", src, re.S),
    "HR firmware name is defined":
        re.search(r'IWX_SO_A_HR_B_FW\s+"iwx-so-a0-hr-b0-77"', var),
}
failed = []
for label, ok in checks.items():
    print(("PASS" if ok else "FAIL") + "\t" + label)
    if not ok:
        failed.append(label)
if failed:
    raise SystemExit("AX101 source gate failed: " + "; ".join(failed))
print("AX101_STATIC_SOURCE_GATE=PASS")
PY
