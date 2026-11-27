#!/usr/bin/env python3
"""Independent verification of AX101-OpenIndiana-RUN107-SCANFIX-OFFLINE.zip."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import stat
import zipfile

EXPECTED_FIRMWARE = "bed4958236366c3c451b3ec994a56b2ad83c08d70e0a1eae42fb00f2eaabbcc8"

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def need(ok: bool, message: str) -> None:
    if not ok:
        raise SystemExit("OFFLINE ZIP VERIFY FAIL: " + message)

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("archive", type=Path)
    args = ap.parse_args()
    with zipfile.ZipFile(args.archive) as z:
        bad = z.testzip()
        need(bad is None, "ZIP CRC failure: " + str(bad))
        entries = {x.filename: x for x in z.infolist()}
        required = (
            "payload/iwx",
            "payload/iwx-so-a0-hr-b0-77",
            "LICENSE.iwlwifi_firmware",
            "verify-bundle.sh",
            "preflight.sh",
            "install-manual.sh",
            "register-driver.sh",
            "diagnose-ax101.sh",
            "rollback.sh",
            "connect-wifi.py",
            "build-provenance.txt",
            "manifest.json",
            "SHA256SUMS",
            "evidence/guest-artifacts.tar.gz",
            "evidence/postbuild-provenance-verified.txt",
            "evidence/iso-provenance-verified.txt",
            "source/scanfix-implementation.tar.gz",
            "source/scanfix-implementation/patches/0028-ax101-scan-notification-rx-fix.patch",
            "source/scanfix-implementation/tests/scan-regression.py",
            "source/scanfix-implementation/diagnostics/ax101-scan.d",
            "source/scanfix-implementation/tests/run-ax101-scan-validation.sh",
            "evidence/scan-regression-final.log",
            "evidence/scanfix-source-identity.txt",
        )
        for name in required:
            need(name in entries, "required entry missing: " + name)
        sums = z.read("SHA256SUMS").decode().splitlines()
        seen = set()
        for line in sums:
            if not line.strip():
                continue
            expected, rel = line.split("  ", 1)
            need(rel in entries, "SHA256SUMS references absent member: " + rel)
            actual = sha(z.read(rel))
            need(actual == expected, "SHA-256 mismatch: " + rel)
            seen.add(rel)
        need(seen == set(entries) - {"SHA256SUMS"},
             "SHA256SUMS coverage does not equal all non-manifest members")
        module = z.read("payload/iwx")
        firmware = z.read("payload/iwx-so-a0-hr-b0-77")
        provenance = z.read("build-provenance.txt").decode()
        module_hash_marker = next((x.split("=", 1)[1] for x in provenance.splitlines()
                                   if x.startswith("MODULE_SHA256=")), None)
        need(module_hash_marker is not None and sha(module) == module_hash_marker,
             "module SHA-256 does not match build provenance")
        need(sha(firmware) == EXPECTED_FIRMWARE, "firmware SHA-256 mismatch")
        scan_log = z.read("evidence/scan-regression-final.log").decode(errors="replace")
        need("Ran 35 tests" in scan_log and "OK" in scan_log,
             "scan regression result is not 35/35 PASS")
        need("SCANFIX_REGRESSION_TESTS=35/35" in provenance,
             "scan regression result missing from provenance")
        need("SCANFIX_NEGATIVE_CONTROLS=10/10" in provenance,
             "scanfix negative controls missing from provenance")
        need(module[:4] == b"\x7fELF" and module[4] == 2 and module[5] == 1,
             "module is not ELF64 little-endian")
        need(int.from_bytes(module[18:20], "little") == 62,
             "module is not EM_X86_64")
        rollback = z.read("rollback.sh").decode()
        need("print $1; exit" in rollback,
             "rollback does not select the actual module ID from modinfo column 1")
        need("print $2; exit" not in rollback,
             "rollback incorrectly uses modinfo Loadcnt as module ID")
        operational_scripts = {
            "verify-bundle.sh", "preflight.sh", "install-manual.sh",
            "register-driver.sh", "diagnose-ax101.sh", "rollback.sh",
            "connect-wifi.py"
        }
        for name, zi in entries.items():
            mode = (zi.external_attr >> 16) & 0o777
            if name in operational_scripts:
                need(bool(mode & stat.S_IXUSR), "executable bit missing: " + name)
        p = provenance
        for marker in (
            "OPENINDIANA_GUEST_KERNEL=illumos-c893fd03ad",
            "TARGET_LAPTOP_KERNEL=illumos-c893fd03ad",
            "KERNEL_MATCH=PASS",
            "FIRMWARE_API=77",
            "PKG_UPDATE=SKIPPED",
            "ELF_TEXTREL_PRESENT=YES",
            "PHYSICAL_PCI_ATTACH=USER_REPORTED_VERIFIED",
            "PHYSICAL_SCAN_DISCOVERY=NOT_YET_VERIFIED",
        ):
            need(marker in p, "build provenance missing: " + marker)
    print("ZIP_CRC=PASS")
    print("SHA256SUMS=PASS")
    print("FILE_MODES=PASS")
    print("MODULE_SHA256=PASS")
    print("FIRMWARE_SHA256=PASS")
    print("ELF_FORMAT=PASS")
    print("KERNEL_PROVENANCE=PASS")
    print("ROLLBACK_SCRIPT_INCLUDED=PASS")
    print("SCANFIX_SOURCE_AND_TESTS=PASS")
    print("SCANFIX_REGRESSION=35/35 PASS")
    print("OFFLINE_BUNDLE_VERIFY=PASS")

if __name__ == "__main__":
    main()
