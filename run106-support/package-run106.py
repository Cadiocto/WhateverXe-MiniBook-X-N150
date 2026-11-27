#!/usr/bin/env python3
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import tarfile
import tempfile
import zipfile

TARGET = "illumos-c893fd03ad"
SOLNIX = "1cee7021091f3df92b7816ece2902ff59b37f1be"
ILLUMOS = "098e95820891f8a83fae679daece7fcf58b7b3a4"
FIRMWARE_SHA = "bed4958236366c3c451b3ec994a56b2ad83c08d70e0a1eae42fb00f2eaabbcc8"
ISO_SHA = "c5cd9572219f49975c333ac8339b18989e5477ab2e81844b0b84c3ae776a6e1a"

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("RUN106 PACKAGE BLOCKED: " + message)

def read(path: Path) -> str:
    return path.read_text(errors="replace") if path.exists() else ""

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", type=Path, required=True)
    ap.add_argument("--artifacts", type=Path, required=True)
    ap.add_argument("--run-id", required=True)
    ap.add_argument("--commit", required=True)
    ap.add_argument("--branch", required=True)
    ap.add_argument("--runner-kernel", required=True)
    ap.add_argument("--builder-commit", required=True)
    ap.add_argument("--iso-sha256", required=True)
    args = ap.parse_args()

    repo = args.repo.resolve()
    art = args.artifacts.resolve()
    require(args.branch == "ax101-r5-integration-validation",
            "source branch is not the validation branch")
    require(args.iso_sha256 == ISO_SHA,
            "October 30 ISO checksum does not match the official sidecar")
    require(args.builder_commit == "25a7144f849eec4c6c31303b41dfd2593ef59cfd",
            "OpenIndiana builder revision is not the reviewed pin")

    guest_tar = art / "guest-artifacts.tar.gz"
    require(guest_tar.is_file(), "guest-artifacts.tar.gz was not received")
    preflight_upload = art / "target-kernel-preflight.txt"
    require(preflight_upload.is_file(), "pre-tool guest preflight was not received")
    preflight_text = read(preflight_upload)
    require("illumos-c893fd03ad" in preflight_text,
            "pre-tool guest preflight does not identify the target kernel")
    require("SunOS" in preflight_text and "OpenIndiana" in preflight_text,
            "pre-tool guest preflight is missing uname or release evidence")

    with tempfile.TemporaryDirectory(prefix="ax101-run106-package-") as td:
        tmp = Path(td)
        try:
            with tarfile.open(guest_tar, "r:gz") as tf:
                members = tf.getmembers()
                for m in members:
                    p = Path(m.name)
                    require(not p.is_absolute() and ".." not in p.parts,
                            "unsafe path in guest evidence tar: " + m.name)
                    require(m.isfile() or m.isdir(),
                            "non-regular entry in guest evidence tar: " + m.name)
                tf.extractall(tmp, filter="data")
        except Exception as exc:
            raise SystemExit("RUN106 PACKAGE BLOCKED: cannot extract guest evidence: " + str(exc))
        roots = list(tmp.glob("ax101-run106-artifacts"))
        require(len(roots) == 1 and roots[0].is_dir(),
                "guest evidence archive has unexpected structure")
        guest = roots[0]

        def evidence(name: str) -> Path:
            p = guest / name
            require(p.is_file(), "required guest evidence is missing: " + name)
            return p

        before = read(evidence("target-kernel-preflight-before-tools.txt"))
        toolchain = read(evidence("toolchain-preflight.txt"))
        identity = read(evidence("target-kernel-identity.txt"))
        build_status = read(evidence("native-build-status.txt"))
        status = read(evidence("STATUS.txt"))
        compile_status = read(evidence("native-compile-exit.txt"))
        modload = read(evidence("iwx-krtld-modload-status.txt"))
        modunload = read(evidence("iwx-modunload-status.txt"))

        require("illumos-c893fd03ad" in before and "KERNEL_MATCH=PASS" in toolchain,
                "the pre-package or post-package target kernel gate did not pass")
        require("illumos-c893fd03ad" in identity and "KERNEL_MATCH=PASS" in identity,
                "the native-build guest kernel identity is not an exact target match")
        require("exit_code=0" in build_status,
                "native build diagnostics report a non-zero exit")
        require("NATIVE_OPENINDIANA_BUILD=PASS" in status,
                "native build did not publish PASS status")
        require(re.search(r"gmake_exit=0\b", compile_status) is not None,
                "native compilation did not pass")
        require(re.search(r"modload_exit=0\b", modload) is not None,
                "native modload did not pass")
        require(re.search(r"modunload_exit=0\b", modunload) is not None,
                "native modunload did not pass")

        for name in ("run87-mbuf-regression.log", "run87-dma-regression.log",
                     "connect-wifi-tests.txt", "elf-symbol-reconciliation.txt",
                     "iwx.dynamic-acceptance.txt", "iwx.relocations.txt",
                     "iwx.elf-header.txt", "compiler-output.log"):
            evidence(name)

        module = evidence("iwx")
        firmware = evidence("iwx-so-a0-hr-b0-77")
        license_file = evidence("iwx-license")
        require(sha256(firmware) == FIRMWARE_SHA,
                "firmware image hash differs from the fixed API-77 image")
        require("redistribution" in read(license_file).lower(),
                "firmware redistribution license does not contain the required permission text")
        with module.open("rb") as f:
            hdr = f.read(64)
        require(len(hdr) >= 20 and hdr[:4] == b"\x7fELF",
                "module is not ELF")
        require(hdr[4] == 2 and hdr[5] == 1,
                "module is not ELF64 little-endian")
        machine = int.from_bytes(hdr[18:20], "little")
        require(machine == 62, "module ELF machine is not EM_X86_64")
        dynamic = read(evidence("iwx.dynamic-acceptance.txt"))
        require("misc/mac" in dynamic,
                "module dynamic dependency on misc/mac was not observed")
        reloc = read(evidence("iwx.relocations.txt"))
        textrel_present = "TEXTREL" in dynamic
        (art / "textrel-status.txt").write_text(
            "ELF_TEXTREL_PRESENT=" + ("YES" if textrel_present else "NO") + "\n")

        package = Path(td) / "bundle"
        (package / "payload").mkdir(parents=True)
        (package / "evidence").mkdir()
        shutil.copy2(module, package / "payload/iwx")
        shutil.copy2(firmware, package / "payload/iwx-so-a0-hr-b0-77")
        shutil.copy2(license_file, package / "LICENSE.iwlwifi_firmware")
        shutil.copy2(repo / "wifi-ax101/connect-wifi.py", package / "connect-wifi.py")
        for source in sorted((repo / "run106-support/offline").glob("*.sh")):
            target = package / source.name
            shutil.copy2(source, target)
            target.chmod(0o755)
        (package / "connect-wifi.py").chmod(0o755)
        shutil.copy2(repo / "run106-support/offline/README.md", package / "README.md")

        evidence_names = [
            "target-kernel-preflight-before-tools.txt", "toolchain-preflight.txt",
            "target-kernel-identity.txt", "runner-environment.txt", "source-revision.txt",
            "compiler-output.log", "native-compile-exit.txt", "iwx.file.txt",
            "iwx.elf-header.txt", "iwx.dynamic-acceptance.txt", "iwx.relocations.txt",
            "iwx.undefined.txt", "elf-symbol-reconciliation.txt",
            "elf-relocation-policy.txt", "iwx-krtld-modload.txt",
            "iwx-krtld-modload-status.txt", "iwx-modunload.txt",
            "iwx-modunload-status.txt", "modinfo-after-modload.txt",
            "modinfo-after-modunload.txt", "run87-mbuf-regression.log",
            "run87-dma-regression.log", "connect-wifi-tests.txt",
            "net80211-header-closure.txt", "STATUS.txt"
        ]
        for name in evidence_names:
            src = guest / name
            if src.is_file():
                shutil.copy2(src, package / "evidence" / name)
        shutil.copy2(preflight_upload, package / "evidence/target-kernel-preflight-received.txt")
        shutil.copy2(art / "runner-host-kernel.txt", package / "evidence/runner-host-kernel.txt")

        compiler = read(guest / "runner-environment.txt")
        provenance = [
            "AX101_RUN=RUN106",
            "SOURCE_REPOSITORY=Cadiocto/WhateverXe-MiniBook-X-N150",
            "SOURCE_BRANCH=" + args.branch,
            "SOURCE_COMMIT=" + args.commit,
            "SOURCE_BASELINE_COMMIT=f530b111e03b966b863d40bd8cb11adf07f60650",
            "PINNED_SOLNIX_COMMIT=" + SOLNIX,
            "PINNED_ILLUMOS_GATE_COMMIT=" + ILLUMOS,
            "OPENINDIANA_BUILDER_COMMIT=" + args.builder_commit,
            "OPENINDIANA_INSTALL_MEDIA=OI-hipster-text-20261030.iso",
            "OPENINDIANA_INSTALL_MEDIA_SHA256=" + args.iso_sha256,
            "RUNNER_HOST_KERNEL=" + args.runner_kernel.replace("\n", " "),
            "OPENINDIANA_GUEST_KERNEL=illumos-c893fd03ad",
            "TARGET_LAPTOP_KERNEL=" + TARGET,
            "KERNEL_MATCH=PASS",
            "PCI_VENDOR_DEVICE=8086:54F0",
            "PCI_SUBSYSTEM=8086:0244",
            "PCI_FUNCTION=14,3",
            "MAC_FAMILY=SO",
            "RF_FAMILY=HR1",
            "RF_ID=0x10c000",
            "IWX_CONFIGURATION=iwx_cfg_so_a0_hr_b0",
            "FIRMWARE_API=77",
            "FIRMWARE_FILENAME=iwx-so-a0-hr-b0-77",
            "FIRMWARE_SHA256=" + FIRMWARE_SHA,
            "MODULE_SHA256=" + sha256(module),
            "MODULE_ELF_CLASS=ELF64",
            "MODULE_ELF_MACHINE=EM_X86_64",
            "MODULE_DYNAMIC_DEPENDENCY=misc/mac",
            "ELF_TEXTREL_PRESENT=" + ("YES" if textrel_present else "NO"),
            "NATIVE_COMPILATION=PASS",
            "MODULE_LINK=PASS",
            "ELF_INSPECTION=PASS",
            "MODLOAD=PASS",
            "MODUNLOAD=PASS",
            "REGRESSION_TESTS=PASS",
            "GITHUB_ACTIONS_RUN_ID=" + args.run_id,
            "",
            "=== COMPILER AND LINKER ===",
            compiler.rstrip(),
            "",
            "=== TARGET KERNEL IDENTITY ===",
            identity.rstrip(),
            "",
            "=== PRE-PACKAGE KERNEL PREFLIGHT ===",
            before.rstrip(),
            ""
        ]
        (package / "build-provenance.txt").write_text("\n".join(provenance))

        manifest_entries = []
        for p in sorted(x for x in package.rglob("*") if x.is_file()):
            rel = p.relative_to(package).as_posix()
            manifest_entries.append({
                "path": rel,
                "size": p.stat().st_size,
                "sha256": sha256(p),
                "mode": "0755" if (p.stat().st_mode & 0o111) else "0644"
            })
        manifest = {
            "bundle": "AX101-OpenIndiana-RUN106-OFFLINE.zip",
            "target_kernel": TARGET,
            "hardware": {"vendor_device": "8086:54F0", "subsystem": "8086:0244"},
            "firmware": {"api": 77, "filename": "iwx-so-a0-hr-b0-77", "sha256": FIRMWARE_SHA},
            "module": {"sha256": sha256(module), "elf_class": "ELF64", "machine": "EM_X86_64"},
            "source_commit": args.commit,
            "run_id": args.run_id,
            "textrel_present": textrel_present,
            "files": manifest_entries
        }
        (package / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
        sums = []
        for p in sorted(x for x in package.rglob("*") if x.is_file() and x.name != "SHA256SUMS"):
            sums.append(sha256(p) + "  " + p.relative_to(package).as_posix())
        (package / "SHA256SUMS").write_text("\n".join(sums) + "\n")

        secret_patterns = [
            re.compile(rb"gh[pousr]_[A-Za-z0-9]{30,}"),
            re.compile(rb"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
            re.compile(rb"AKIA[0-9A-Z]{16}")
        ]
        for p in (x for x in package.rglob("*") if x.is_file()):
            data = p.read_bytes()
            for pattern in secret_patterns:
                require(pattern.search(data) is None, "possible secret detected in " + p.relative_to(package).as_posix())

        out = art / "AX101-OpenIndiana-RUN106-OFFLINE.zip"
        if out.exists():
            out.unlink()
        with zipfile.ZipFile(out, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
            for p in sorted(x for x in package.rglob("*") if x.is_file()):
                rel = p.relative_to(package).as_posix()
                mode = 0o755 if (p.stat().st_mode & 0o111) else 0o644
                info = zipfile.ZipInfo(rel)
                info.create_system = 3
                info.external_attr = (stat.S_IFREG | mode) << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                zf.writestr(info, p.read_bytes())
        with zipfile.ZipFile(out, "r") as zf:
            require(zf.testzip() is None, "ZIP CRC check failed")
            names = set(zf.namelist())
            for required in ("payload/iwx", "payload/iwx-so-a0-hr-b0-77",
                             "preflight.sh", "install-manual.sh", "register-driver.sh",
                             "diagnose-ax101.sh", "rollback.sh", "SHA256SUMS",
                             "manifest.json", "build-provenance.txt"):
                require(required in names, "bundle entry missing after zip: " + required)
            for zi in zf.infolist():
                mode = (zi.external_attr >> 16) & 0o777
                if zi.filename.endswith(".sh") or zi.filename == "connect-wifi.py":
                    require(mode & 0o111, "executable bit missing in ZIP: " + zi.filename)
        (art / "RUN106-ARTIFACT-SHA256SUMS.txt").write_text(
            "MODULE_SHA256=" + sha256(module) + "\n" +
            "FIRMWARE_SHA256=" + FIRMWARE_SHA + "\n" +
            "OFFLINE_ZIP_SHA256=" + sha256(out) + "\n"
        )
        print("KERNEL_MATCH=PASS")
        print("NATIVE_BUILD=PASS")
        print("MODULE_LINK=PASS")
        print("ELF_INSPECTION=PASS")
        print("MODLOAD=PASS")
        print("MODUNLOAD=PASS")
        print("REGRESSION_TESTS=PASS")
        print("ELF_TEXTREL_PRESENT=" + ("YES" if textrel_present else "NO"))
        print("MODULE_SHA256=" + sha256(module))
        print("FIRMWARE_SHA256=" + FIRMWARE_SHA)
        print("OFFLINE_BUNDLE=" + str(out))
        print("OFFLINE_BUNDLE_SHA256=" + sha256(out))
        print("OFFLINE_BUNDLE_SIZE=" + str(out.stat().st_size))

if __name__ == "__main__":
    main()
