#!/usr/bin/env python3
"""Reconcile iwx undefined symbols against the target OpenIndiana kernel exports."""
import collections
import pathlib
import sys

if len(sys.argv) != 4:
    raise SystemExit("usage: check-elf-exports.py <undefined.nm> <kernel.nm> <mac.nm>")

def read_nm(path):
    undefined = set()
    defined = set()
    for raw in pathlib.Path(path).read_text(errors="replace").splitlines():
        fields = raw.split()
        if len(fields) < 2:
            continue
        if fields[0] in ("U", "u") and len(fields) >= 2:
            undefined.add(fields[1])
        elif len(fields) >= 3 and fields[-2] in ("U", "u"):
            undefined.add(fields[-1])
        elif len(fields) >= 3:
            kind = fields[-2]
            if kind not in ("?", "U", "u"):
                defined.add(fields[-1])
    return undefined, defined

undefined, _ = read_nm(sys.argv[1])
_, kernel = read_nm(sys.argv[2])
_, mac = read_nm(sys.argv[3])
exports = kernel | mac

def category(name):
    if name.startswith("ddi_"):
        return "ddi_*"
    if name.startswith("mac_"):
        return "mac_*"
    if name.startswith("mod_"):
        return "mod_*"
    if name in ("allocb", "freeb"):
        return "allocb/freeb"
    if name.startswith("firmware_") or name == "free_firmware":
        return "firmware_*"
    if name.startswith("__stack_chk"):
        return "stack-protector"
    return "other"

groups = collections.defaultdict(list)
missing = []
for symbol in sorted(undefined):
    groups[category(symbol)].append(symbol)
    if symbol not in exports:
        missing.append(symbol)

print("UNDEFINED_SYMBOL_COUNT=%d" % len(undefined))
print("TARGET_EXPORT_COUNT=%d" % len(exports))
for group in ("ddi_*", "mac_*", "mod_*", "allocb/freeb",
              "firmware_*", "stack-protector", "other"):
    values = groups[group]
    print("[%s] %d" % (group, len(values)))
    for symbol in values:
        print("%s\t%s" % (symbol, "RESOLVED" if symbol in exports else "MISSING"))
print("UNRESOLVED_COUNT=%d" % len(missing))
if missing:
    print("UNRESOLVED_SYMBOLS:")
    for symbol in missing:
        print(symbol)
    raise SystemExit(1)
print("ELF_TARGET_EXPORT_RECONCILIATION=PASS")
