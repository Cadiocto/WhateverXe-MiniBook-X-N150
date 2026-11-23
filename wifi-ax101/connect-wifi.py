#!/usr/bin/env python3
"""Connect an OpenIndiana GLDv3 iwx link to WPA2-Personal.

Derives the 32-byte PMK locally and sends the driver's private _wpa_psk MAC
property. The plaintext passphrase is never passed as a command-line argument.
"""
import argparse
import getpass
import hashlib
import re
import shutil
import subprocess
import sys
from typing import Optional


def ssid_bytes(value: str) -> bytes:
    raw = value.encode("utf-8")
    if not 1 <= len(raw) <= 32:
        raise ValueError("SSID must encode to 1..32 UTF-8 bytes")
    if any(c in raw for c in (b"\x00", b"\n", b"\r")):
        raise ValueError("SSID cannot contain NUL or newline bytes")
    return raw


def derive_pmk(passphrase: str, ssid: bytes) -> bytes:
    password = passphrase.encode("utf-8")
    if not 8 <= len(password) <= 63:
        raise ValueError("WPA2 passphrase must encode to 8..63 bytes")
    if not 1 <= len(ssid) <= 32:
        raise ValueError("SSID must contain 1..32 bytes")
    return hashlib.pbkdf2_hmac("sha1", password, ssid, 4096, dklen=32)


def property_value(pmk: bytes, ssid: bytes, bssid: Optional[str] = None) -> str:
    if len(pmk) != 32:
        raise ValueError("PMK must be exactly 32 bytes")
    pin = "-" if bssid is None else bssid.lower()
    if pin != "-" and not re.fullmatch(r"(?:[0-9a-f]{2}:){5}[0-9a-f]{2}", pin):
        raise ValueError("BSSID must have the form aa:bb:cc:dd:ee:ff")
    if not 1 <= len(ssid) <= 32 or any(c in ssid for c in (b"\x00", b"\n", b"\r")):
        raise ValueError("SSID must be 1..32 bytes and contain no NUL/newline")
    # Kernel parser fixes the PMK/BSSID offsets; SSID may contain colons.
    return pmk.hex() + ":" + pin + ":" + ssid.decode("utf-8")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ssid", help="Wi-Fi SSID (UTF-8, 1..32 bytes)")
    parser.add_argument("--interface", default="iwx0", help="GLDv3 link name")
    parser.add_argument("--bssid", help="optional AP pin, aa:bb:cc:dd:ee:ff")
    args = parser.parse_args(argv)
    try:
        ssid = ssid_bytes(args.ssid)
        if not re.fullmatch(r"[A-Za-z0-9_.-]{1,32}", args.interface):
            raise ValueError("interface name contains unsupported characters")
        if args.bssid is not None and not re.fullmatch(
            r"(?:[0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}", args.bssid
        ):
            raise ValueError("BSSID must have the form aa:bb:cc:dd:ee:ff")
        dladm = shutil.which("dladm") or "/usr/sbin/dladm"
        passphrase = getpass.getpass("WPA2 passphrase: ")
        pmk = derive_pmk(passphrase, ssid)
        value = property_value(pmk, ssid, args.bssid)
        # -t is deliberately temporary; this does not persist credentials.
        return subprocess.run(
            [dladm, "set-linkprop", "-t", "-p", "_wpa_psk=" + value, args.interface],
            check=False,
        ).returncode
    except (ValueError, OSError) as exc:
        print(f"connect-wifi: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
