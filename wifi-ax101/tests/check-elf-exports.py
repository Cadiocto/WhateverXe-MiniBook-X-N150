#!/usr/bin/env python3
"""Inventory iwx undefined symbols against selected OpenIndiana kernel exports.

The final symbol-resolution authority is kldd/krtld. This report inventories
undefined symbols and coverage in the explicitly inspected kernel and MAC
export tables; it does not mistake symbols supplied by other kernel modules
for unresolved references.
"""
import collections
import pathlib
import sys


def read_undefined_nm(path):
    lines = pathlib.Path(path).read_text(errors="replace").splitlines()
    undefined = set()
    solaris_header = any(line.strip().startswith("Undefined symbols from ")
                         for line in lines)
    if solaris_header:
        in_list = False
        for raw in lines:
            line = raw.strip()
            if line.startswith("Undefined symbols from "):
                in_list = True
                continue
            if not in_list or not line:
                continue
            fields = line.split()
            if len(fields) == 1:
                undefined.add(fields[0])
        if not undefined:
            raise ValueError("Solaris nm -u header found but no undefined symbols parsed")
        return undefined

    for raw in lines:
        fields = raw.split()
        if len(fields) >= 2 and fields[0] in ("U", "u"):
            undefined.add(fields[1])
        elif len(fields) >= 3 and fields[-2] in ("U", "u"):
            undefined.add(fields[-1])
    if not undefined:
        raise ValueError("no undefined symbols parsed; nm output format is unrecognized")
    return undefined


def read_defined_nm(path):
    defined = set()
    for raw in pathlib.Path(path).read_text(errors="replace").splitlines():
        fields = raw.split()
        if len(fields) >= 3 and fields[-2] not in ("?", "U", "u"):
            defined.add(fields[-1])
    if not defined:
        raise ValueError("no defined symbols parsed from export table: " + path)
    return defined


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


def main(argv):
    if len(argv) != 4:
        print("usage: check-elf-exports.py <undefined.nm> <kernel.nm> <mac.nm>",
              file=sys.stderr)
        return 2
    try:
        undefined = read_undefined_nm(argv[1])
        exports = read_defined_nm(argv[2]) | read_defined_nm(argv[3])
    except (OSError, ValueError) as exc:
        print("ERROR: %s" % exc, file=sys.stderr)
        return 2

    groups = collections.defaultdict(list)
    for symbol in sorted(undefined):
        groups[category(symbol)].append(symbol)
    inspected = sorted(symbol for symbol in undefined if symbol in exports)
    not_inspected = sorted(symbol for symbol in undefined if symbol not in exports)

    print("UNDEFINED_SYMBOL_COUNT=%d" % len(undefined))
    print("TARGET_EXPORT_COUNT=%d" % len(exports))
    print("RESOLVED_IN_INSPECTED_EXPORTS=%d" % len(inspected))
    print("NOT_IN_INSPECTED_EXPORTS=%d" % len(not_inspected))
    for group in ("ddi_*", "mac_*", "mod_*", "allocb/freeb",
                  "firmware_*", "stack-protector", "other"):
        values = groups[group]
        print("[%s] %d" % (group, len(values)))
        for symbol in values:
            state = "FOUND" if symbol in exports else "NOT_IN_INSPECTED_TABLES"
            print("%s\t%s" % (symbol, state))
    print("ELF_SYMBOL_INVENTORY=PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
