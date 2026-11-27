#!/usr/bin/env python3
"""Independent verification of AX101-OpenIndiana-RUN106-OFFLINE.zip."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import stat
import zipfile

EXPECTED_MODULE = "8fa07fedbf0e4549979f5e8ddeb3f8410b4083e3b4428653bb7f763d5458b049"
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
        need(sha(module) == EXPECTED_MODULE, "module SHA-256 mismatch")
        need(sha(firmware) == EXPECTED_FIRMWARE, "firmware SHA-256 mismatch")
        need(module[:4] == b"\x7fELF" and module[4] == 2 and module[5] == 1,
             "module is not ELF64 little-endian")
        need(int.from_bytes(module[18:20], "little") == 62,
             "module is not EM_X86_64")
        for name, zi in entries.items():
            mode = (zi.external_attr >> 16) & 0o777
            if name.endswith(".sh") or name == "connect-wifi.py":
                need(bool(mode & stat.S_IXUSR), "executable bit missing: " + name)
        p = z.read("build-provenance.txt").decode()
        for marker in (
            "OPENINDIANA_GUEST_KERNEL=illumos-c893fd03ad",
            "TARGET_LAPTOP_KERNEL=illumos-c893fd03ad",
            "KERNEL_MATCH=PASS",
            "FIRMWARE_API=77",
            "PKG_UPDATE=SKIPPED",
            "ELF_TEXTREL_PRESENT=YES",
            "PHYSICAL_PCI_ATTACH=NOT_PROVEN",
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
    print("OFFLINE_BUNDLE_VERIFY=PASS")

if __name__ == "__main__":
    main()
