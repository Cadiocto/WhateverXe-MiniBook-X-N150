#!/usr/bin/env python3
import importlib.util
import pathlib
import unittest

SCRIPT = pathlib.Path(__file__).resolve().parents[1] / "connect-wifi.py"
SPEC = importlib.util.spec_from_file_location("connect_wifi", SCRIPT)
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


class ConnectWifiTests(unittest.TestCase):
    def test_rfc_wpa_pbkdf2_vector(self):
        self.assertEqual(
            MOD.derive_pmk("password", b"IEEE").hex(),
            "f42c6fc52df0ebef9ebb4b90b38a5f902e83fe1b135a70e23aed762e9710a12e",
        )

    def test_property_without_bssid(self):
        self.assertEqual(
            MOD.property_value(bytes(range(32)), b"home"),
            bytes(range(32)).hex() + ":-:home",
        )

    def test_property_with_bssid_and_colon_ssid(self):
        self.assertEqual(
            MOD.property_value(bytes(32), b"a:b", "AA:BB:CC:DD:EE:FF"),
            bytes(32).hex() + ":aa:bb:cc:dd:ee:ff:a:b",
        )

    def test_reject_short_passphrase(self):
        with self.assertRaises(ValueError):
            MOD.derive_pmk("short", b"home")

    def test_reject_invalid_ssid_length(self):
        with self.assertRaises(ValueError):
            MOD.ssid_bytes("x" * 33)

    def test_reject_bad_bssid(self):
        with self.assertRaises(ValueError):
            MOD.property_value(bytes(32), b"home", "not-a-bssid")


if __name__ == "__main__":
    unittest.main(verbosity=2)
