#!/usr/bin/env python3
"""Add experimental Intel 8086:54C8 entry derived from upstream 51C8."""
import plistlib
from pathlib import Path

path = Path("Resources/Controllers.plist")
with path.open("rb") as f:
    controllers = plistlib.load(f)

if not isinstance(controllers, list):
    raise SystemExit("STOP: Controllers.plist root is not an array")

matches = [x for x in controllers if x.get("Vendor") == "Intel" and x.get("Device") == 20936]
if len(matches) != 1:
    raise SystemExit(f"STOP: expected one Intel 0x51C8 entry, found {len(matches)}")

existing = [x for x in controllers if x.get("Vendor") == "Intel" and x.get("Device") == 21704]
if existing:
    raise SystemExit("STOP: Intel 0x54C8 entry already exists; refusing duplicate")

entry = dict(matches[0])
entry["Device"] = 21704
entry["Name"] = "Intel Alder Lake-N / Twin Lake HDA Controller (0x54C8) — experimental"
controllers.insert(controllers.index(matches[0]) + 1, entry)

with path.open("wb") as f:
    plistlib.dump(controllers, f, fmt=plistlib.FMT_XML, sort_keys=False)

# Re-open and validate the generated plist and the exact copied patch data.
with path.open("rb") as f:
    check = plistlib.load(f)
new = [x for x in check if x.get("Vendor") == "Intel" and x.get("Device") == 21704]
assert len(new) == 1
assert new[0]["Patches"] == matches[0]["Patches"]
print("Added experimental Intel 0x54C8 entry copied from upstream 0x51C8.")
print("WARNING: build success does not prove binary patch compatibility on 0x54C8.")
