#!/usr/bin/env python3
"""Verify and reconstruct the missing RUN106 provenance handoff from its immutable job log."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

BUILDER_REPO = "https://github.com/anyvm-org/openindiana-builder.git"
BUILDER_COMMIT = "25a7144f849eec4c6c31303b41dfd2593ef59cfd"
ORIGINAL_GIT_BLOB = "557c8e0217e5dc69612022d5b1e6c57cb13ca6ec"
MODIFIED_SHA256 = "f014902780cae2a5c00f3309ed728c802cc664257d7a025e28432bcc000e4c26"
ISO_URL = "https://dlc.openindiana.org/isos/hipster/20261030/OI-hipster-text-20261030.iso"
ISO_SHA256 = "c5cd9572219f49975c333ac8339b18989e5477ab2e81844b0b84c3ae776a6e1a"
RUN_ID = "46059925320"
SOURCE_COMMIT = "dc295a86d786f3b693e944a5e61b6b2a4c9e8f37"
SOURCE_ARTIFACT_ID = "15229533467"
SOURCE_ARTIFACT_SHA256 = "107ce8e4c2381d5c91dece0d0b2ea29539c49389d177af605882ee2a20880f11"
SKIP_TEXT = "RUN106: pkg update intentionally skipped to preserve the verified October 30 kernel"

def fail(message: str) -> None:
    raise SystemExit("RUN106 PROVENANCE BLOCKED: " + message)

def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def run(args: list[str], cwd: Path | None = None) -> str:
    p = subprocess.run(args, cwd=cwd, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    if p.returncode:
        fail("command failed: " + " ".join(args) + "\n" + p.stdout[-4000:])
    return p.stdout.strip()

def parse_kv(text: str) -> dict[str, str]:
    out = {}
    for line in text.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            out[k.strip()] = v.strip()
    return out

def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--artifact-dir", type=Path, required=True)
    ap.add_argument("--job-log", type=Path, required=True)
    ap.add_argument("--evidence-dir", type=Path, required=True)
    ap.add_argument("--source-artifact-sha256", default=SOURCE_ARTIFACT_SHA256)
    args = ap.parse_args()

    art = args.artifact_dir.resolve()
    log_path = args.job_log.resolve()
    evidence = args.evidence_dir.resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    prep_path = art / "builder-preparation.txt"
    if not prep_path.is_file() or not log_path.is_file():
        fail("builder-preparation.txt or immutable GitHub Actions job log is missing")
    prep = parse_kv(prep_path.read_text(errors="strict"))
    log = log_path.read_text(errors="replace")

    if args.source_artifact_sha256 != SOURCE_ARTIFACT_SHA256:
        fail("source artifact SHA-256 is not the frozen RUN106 artifact")
    required_prep = {
        "BUILDER_COMMIT": BUILDER_COMMIT,
        "POSTBUILD_ORIGINAL_GIT_BLOB": ORIGINAL_GIT_BLOB,
        "POSTBUILD_MODIFIED_SHA256": MODIFIED_SHA256,
        "ISO_URL": ISO_URL,
        "ISO_SHA256": ISO_SHA256,
        "SOURCE_COMMIT": SOURCE_COMMIT,
        "RUN_ID": RUN_ID,
        "TARGET_KERNEL": "illumos-c893fd03ad",
    }
    for key, value in required_prep.items():
        if prep.get(key) != value:
            fail(f"builder-preparation.txt {key} mismatch: {prep.get(key)!r}")

    if f"RUN_ID: {RUN_ID}" not in log or f"SOURCE_COMMIT: {SOURCE_COMMIT}" not in log:
        fail("Actions log does not identify the original RUN106 commit and run")
    if "hooks/host_beforeBuild.sh" not in log:
        fail("host_beforeBuild hook invocation is absent from the Actions log")
    for text in (
        ISO_URL,
        ISO_SHA256,
        "OPENINDIANA_ISO_VERIFICATION=PASS",
        f"POSTBUILD_ORIGINAL_GIT_BLOB={ORIGINAL_GIT_BLOB}",
        f"POSTBUILD_MODIFIED_SHA256={MODIFIED_SHA256}",
        "PKG_UPDATE=SKIPPED",
        "build/openindiana.iso.download: OK",
    ):
        if text not in log:
            fail("Actions log is missing verified ISO/postBuild evidence: " + text)

    before_hook = log.find("hooks/host_beforeBuild.sh")
    iso_ok = log.find("build/openindiana.iso.download: OK", before_hook)
    post_hook = log.find("hooks/vm_postBuild.sh")
    if min(before_hook, iso_ok, post_hook) < 0 or not (before_hook < iso_ok < post_hook):
        fail("host ISO verification and VM postBuild execution are not in expected order")

    # Require the executed log output, not merely the echo command shown when
    # build.py prints the hook source to the runner log.
    executed_marker = re.search(
        r"^\d{4}-\d\d-\d\dT[^\n]*Z " + re.escape(SKIP_TEXT) + r"$",
        log, re.MULTILINE)
    if not executed_marker or executed_marker.start() <= post_hook:
        fail("postBuild execution output does not contain the exact runtime skip marker")

    # Independently reconstruct the exact postBuild edit from the pinned source.
    with tempfile.TemporaryDirectory(prefix="run106-builder-proof-") as td:
        builder = Path(td) / "builder"
        run(["git", "init", str(builder)])
        run(["git", "remote", "add", "origin", BUILDER_REPO], cwd=builder)
        run(["git", "fetch", "--depth=1", "origin", BUILDER_COMMIT], cwd=builder)
        run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=builder)
        if run(["git", "rev-parse", "HEAD"], cwd=builder) != BUILDER_COMMIT:
            fail("checked out OpenIndiana builder does not match the pinned commit")
        original_path = builder / "hooks/vm_postBuild.sh"
        original = original_path.read_bytes()
        git_blob = run(["git", "hash-object", "hooks/vm_postBuild.sh"], cwd=builder)
        if git_blob != ORIGINAL_GIT_BLOB:
            fail("original postBuild Git blob SHA-1 does not match the reviewed builder")
        old = b"\npkg update\n"
        new = b"\necho 'RUN106: pkg update intentionally skipped to preserve the verified October 30 kernel'\n"
        if original.count(old) != 1:
            fail("pinned postBuild source does not contain exactly one pkg update statement")
        modified = original.replace(old, new, 1)
        modified_sha = sha256_bytes(modified)
        if modified_sha != MODIFIED_SHA256 or prep.get("POSTBUILD_MODIFIED_SHA256") != modified_sha:
            fail("reconstructed postBuild modification SHA-256 does not match RUN106 log")

        (evidence / "vm_postBuild.original.sh").write_bytes(original)
        (evidence / "vm_postBuild.modified.sh").write_bytes(modified)

    # Verify the actual guest pre-tool identity, target-kernel equality and release date.
    preflight = art / "target-kernel-preflight.txt"
    if not preflight.is_file():
        fail("pre-tool target kernel preflight file is missing")
    preflight_text = preflight.read_text(errors="replace")
    for token in (
        "OPENINDIANA_GUEST_KERNEL=illumos-c893fd03ad",
        "TARGET_LAPTOP_KERNEL=illumos-c893fd03ad",
        "KERNEL_MATCH=PASS",
        "Assembled 30 October 2026",
    ):
        if token not in preflight_text:
            fail("pre-tool kernel evidence missing: " + token)

    lines = log.splitlines()
    needles = (
        "hooks/host_beforeBuild.sh",
        f"ISO_URL=\"{ISO_URL}\"",
        f"EXPECTED_SHA256=\"{ISO_SHA256}\"",
        "build/openindiana.iso.download: OK",
        "OPENINDIANA_ISO_VERIFICATION=PASS",
        f"POSTBUILD_ORIGINAL_GIT_BLOB={ORIGINAL_GIT_BLOB}",
        f"POSTBUILD_MODIFIED_SHA256={MODIFIED_SHA256}",
        "PKG_UPDATE=SKIPPED",
        "hooks/vm_postBuild.sh",
        SKIP_TEXT,
    )
    selected = []
    for needle in needles:
        matches = [line for line in lines if needle in line]
        if not matches:
            fail("job log excerpt source line missing: " + needle)
        # For the execution marker, select only the actual timestamped runtime
        # output, not the earlier printed echo statement.
        if needle == SKIP_TEXT:
            matches = [line for line in matches if re.search(r"Z " + re.escape(SKIP_TEXT) + r"$", line)]
            if not matches:
                fail("runtime postBuild marker line not found")
        selected.extend(matches)
    # Preserve source log order and avoid repeating duplicate lines.
    selected = list(dict.fromkeys(selected))
    excerpt = "\n".join(selected) + "\n"
    (evidence / "job-log-provenance-excerpt.txt").write_text(excerpt)

    job_log_sha = sha256_bytes(log_path.read_bytes())
    postbuild_record = [
        "PROVENANCE_STATUS=VERIFIED",
        "RUN_ID=" + RUN_ID,
        "SOURCE_COMMIT=" + SOURCE_COMMIT,
        "BUILDER_COMMIT=" + BUILDER_COMMIT,
        "POSTBUILD_ORIGINAL_GIT_BLOB=" + git_blob,
        "POSTBUILD_MODIFIED_SHA256=" + modified_sha,
        "POSTBUILD_HOOK_INVOKED=PASS",
        "POSTBUILD_HOOK_EXECUTED=PASS",
        "PKG_UPDATE=SKIPPED",
        "RUNTIME_LOG_MARKER=" + SKIP_TEXT,
        "GITHUB_JOB_LOG_SHA256=" + job_log_sha,
        "SOURCE_ARTIFACT_ID=" + SOURCE_ARTIFACT_ID,
        "SOURCE_ARTIFACT_SHA256=" + SOURCE_ARTIFACT_SHA256,
    ]
    postbuild_file = art / "postbuild-provenance-verified.txt"
    postbuild_file.write_text("\n".join(postbuild_record) + "\n")

    iso_record = [
        "ISO_PROVENANCE_STATUS=VERIFIED_FROM_RUN_LOG",
        "ISO_PROVENANCE_FILE_ON_BUILD_RUNNER=build/run106-iso-provenance.txt",
        "ISO_PROVENANCE_HANDOFF=RECONSTRUCTED_FROM_IMMUTABLE_GITHUB_JOB_LOG",
        "OPENINDIANA_ISO_URL=" + ISO_URL,
        "OPENINDIANA_ISO_SHA256=" + ISO_SHA256,
        "OPENINDIANA_ISO_VERIFICATION=PASS",
        "ISO_CHECKSUM_COMMAND_RESULT=build/openindiana.iso.download: OK",
        "BUILDER_COMMIT=" + BUILDER_COMMIT,
        "POSTBUILD_ORIGINAL_GIT_BLOB=" + git_blob,
        "POSTBUILD_MODIFIED_SHA256=" + modified_sha,
        "PKG_UPDATE=SKIPPED",
        "GITHUB_JOB_LOG_SHA256=" + job_log_sha,
    ]
    iso_file = art / "iso-provenance-verified.txt"
    iso_file.write_text("\n".join(iso_record) + "\n")
    shutil.copy2(preflight, evidence / "target-kernel-preflight-received.txt")
    print("\n".join(postbuild_record))
    print("\n".join(iso_record))
    print("RUN106_PROVENANCE_HANDOFF=PASS")

if __name__ == "__main__":
    main()
