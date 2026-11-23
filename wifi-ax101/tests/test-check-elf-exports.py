#!/usr/bin/env python3
"""Regression tests for Solaris and portable nm undefined-symbol output."""
import pathlib
import subprocess
import sys
import tempfile

SCRIPT = pathlib.Path(__file__).with_name("check-elf-exports.py")


def run_case(undefined, kernel, mac):
    with tempfile.TemporaryDirectory() as td:
        root = pathlib.Path(td)
        (root / "undefined.nm").write_text(undefined)
        (root / "kernel.nm").write_text(kernel)
        (root / "mac.nm").write_text(mac)
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(root / "undefined.nm"),
             str(root / "kernel.nm"), str(root / "mac.nm")],
            text=True, capture_output=True)


solaris = run_case(
    "\nUndefined symbols from /tmp/iwx:\n\n    foo\n    bar\n",
    "/kernel/unix:\n00000000000000000001 T foo\n"
    "00000000000000000000 U unresolved_in_kernel\n",
    "/kernel/misc/mac:\n00000000000000000002 T baz\n")
assert solaris.returncode == 0, solaris.stderr
assert "UNDEFINED_SYMBOL_COUNT=2" in solaris.stdout, solaris.stdout
assert "RESOLVED_IN_INSPECTED_EXPORTS=1" in solaris.stdout, solaris.stdout
assert "NOT_IN_INSPECTED_EXPORTS=1" in solaris.stdout, solaris.stdout
assert "ELF_SYMBOL_INVENTORY=PASS" in solaris.stdout, solaris.stdout

portable = run_case(
    "                 U foo\n00000000000000000000 U bar\n",
    "/kernel/unix:\n00000000000000000001 T foo\n",
    "/kernel/misc/mac:\n00000000000000000002 T bar\n")
assert portable.returncode == 0, portable.stderr
assert "UNDEFINED_SYMBOL_COUNT=2" in portable.stdout, portable.stdout
assert "NOT_IN_INSPECTED_EXPORTS=0" in portable.stdout, portable.stdout

unrecognized = run_case(
    "this is not nm undefined output\n",
    "/kernel/unix:\n00000000000000000001 T foo\n",
    "/kernel/misc/mac:\n00000000000000000002 T bar\n")
assert unrecognized.returncode != 0, (
    "unrecognized nm output must not silently pass")
assert "unrecognized" in unrecognized.stderr, unrecognized.stderr

print("PASS: Solaris nm -u bare-symbol format parsed")
print("PASS: portable U-symbol formats parsed")
print("PASS: unknown nm format rejected instead of false zero-symbol PASS")
