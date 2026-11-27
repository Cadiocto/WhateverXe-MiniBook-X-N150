#!/usr/bin/env python3
import subprocess, tempfile
from pathlib import Path
helper = Path(__file__).with_name("native-diagnostics.sh")
def run_case(body, expected, check):
    with tempfile.TemporaryDirectory(prefix="ax101-diag-test-") as td:
        art = Path(td) / "artifacts"
        script = f"""set -Eeuo pipefail
source {str(helper)!r}
ax101_diag_init {str(art)!r}
trap 'rc=$?; trap - EXIT ERR; set +e; ax101_diag_finish "$rc"; exit "$rc"' EXIT
{body}
"""
        p = subprocess.run(["bash", "-c", script], text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if p.returncode != expected: raise SystemExit(f"expected {expected}, got {p.returncode}:\n{p.stdout}")
        check(art, p.stdout)
run_case('ax101_diag_stage POSITIVE; ax101_diag_command success sh -c "echo stdout-ok; echo stderr-ok >&2"; ax101_diag_pass POSITIVE', 0, lambda a,o: ("stdout-ok" in (a/"native-stdout.log").read_text() and "stderr-ok" in (a/"native-stderr.log").read_text()) or (_ for _ in ()).throw(SystemExit("stream capture failed")))
print("PASS command exit 0 and separate stdout/stderr capture")
run_case('ax101_diag_stage NONZERO; ax101_diag_command nonzero sh -c "echo compiler-error >&2; exit 23"', 23, lambda a,o: ("exit_code=23" in (a/"native-build-status.txt").read_text() and "compiler-error" in (a/"failure-summary.txt").read_text()) or (_ for _ in ()).throw(SystemExit("nonzero failure diagnostics lost")))
print("PASS nonzero command and stderr diagnostics preserve exit code")
run_case('ax101_diag_stage PIPEFAIL; false | tee /dev/null', 1, lambda a,o: "exit_code=1" in (a/"native-build-status.txt").read_text())
print("PASS pipefail propagates upstream failure")
run_case('ax101_diag_stage SSH; ax101_diag_command ssh sh -c "echo ssh-failed >&2; exit 255"', 255, lambda a,o: "exit_code=255" in (a/"native-build-status.txt").read_text())
print("PASS SSH failure exit code is preserved")
run_case('ax101_diag_stage REMOTE_LOG_MISSING; rm -f "$AX101_ART/native-openindiana-build.log"; ax101_diag_command failed sh -c "exit 19"', 19, lambda a,o: "exit_code=19" in (a/"native-build-status.txt").read_text())
print("PASS failure status survives absent remote merged log")
with tempfile.TemporaryDirectory(prefix="ax101-diag-cleanup-") as td:
    art=Path(td)/"artifacts"
    script=f"""set -Eeuo pipefail
source {str(helper)!r}
ax101_diag_init {str(art)!r}
trap 'rc=$?; trap - EXIT ERR; set +e; false; ax101_diag_finish "$rc"; exit "$rc"' EXIT
ax101_diag_stage ORIGINAL_FAILURE
exit 37
"""
    p=subprocess.run(["bash","-c",script],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.returncode != 37 or "exit_code=37" not in (art/"native-build-status.txt").read_text(): raise SystemExit("cleanup failure masked original exit code")
print("PASS cleanup failure cannot mask original exit code")
print("NATIVE_DIAGNOSTICS_TESTS=PASS 6/6")
