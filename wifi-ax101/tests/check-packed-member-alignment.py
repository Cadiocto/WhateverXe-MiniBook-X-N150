#!/usr/bin/env python3
"""Guard the packed channel-profile alignment overlay and its regression control."""
import re
import sys
from pathlib import Path


def validate(source):
    failures = []

    def require(pattern, label):
        if re.search(pattern, source, re.S) is None:
            failures.append(label)

    require(
        r"void\s+iwx_init_channel_map\s*\(\s*struct iwx_softc\s*\*sc\s*,"
        r"\s*const uint8_t\s*\*channel_profile_v3\s*,"
        r"\s*const uint8_t\s*\*channel_profile_v4\s*,\s*int\s+nchan_profile\s*\)",
        "channel map must accept byte-oriented profile pointers",
    )
    require(r"uint16_t\s+channel_flags_v3\s*;", "v3 aligned local is missing")
    require(r"uint32_t\s+channel_flags_v4\s*;", "v4 aligned local is missing")
    require(
        r"memcpy\s*\(\s*&channel_flags_v4\s*,\s*channel_profile_v4\s*"
        r"\+\s*ch_idx\s*\*\s*sizeof\s*\(\s*channel_flags_v4\s*\)"
        r"\s*,\s*sizeof\s*\(\s*channel_flags_v4\s*\)\s*\)\s*;"
        r"\s*ch_flags\s*=\s*le32toh\s*\(\s*channel_flags_v4\s*\)\s*;",
        "v4 profile word must be copied to an aligned local before endian conversion",
    )
    require(
        r"memcpy\s*\(\s*&channel_flags_v3\s*,\s*channel_profile_v3\s*"
        r"\+\s*ch_idx\s*\*\s*sizeof\s*\(\s*channel_flags_v3\s*\)"
        r"\s*,\s*sizeof\s*\(\s*channel_flags_v3\s*\)\s*\)\s*;"
        r"\s*ch_flags\s*=\s*le16toh\s*\(\s*channel_flags_v3\s*\)\s*;",
        "v3 profile word must be copied to an aligned local before endian conversion",
    )

    call_sites = (
        r"iwx_init_channel_map\s*\(\s*sc\s*,\s*NULL\s*,\s*\(const uint8_t\s*\*\)\s*rsp\s*"
        r"\+\s*offsetof\s*\(\s*struct iwx_nvm_get_info_rsp\s*,\s*regulatory\s*\)"
        r"\s*\+\s*offsetof\s*\(\s*struct iwx_nvm_get_info_regulatory\s*,\s*channel_profile\s*\)",
        r"iwx_init_channel_map\s*\(\s*sc\s*,\s*\(const uint8_t\s*\*\)\s*rsp_v3\s*"
        r"\+\s*offsetof\s*\(\s*struct iwx_nvm_get_info_rsp_v3\s*,\s*regulatory\s*\)"
        r"\s*\+\s*offsetof\s*\(\s*struct iwx_nvm_get_info_regulatory_v1\s*,\s*channel_profile\s*\)",
        r"iwx_init_channel_map\s*\(\s*sc\s*,\s*NULL\s*,\s*\(const uint8_t\s*\*\)\s*resp\s*"
        r"\+\s*offsetof\s*\(\s*struct iwx_mcc_update_resp_v4\s*,\s*channels\s*\)",
        r"iwx_init_channel_map\s*\(\s*sc\s*,\s*NULL\s*,\s*\(const uint8_t\s*\*\)\s*resp\s*"
        r"\+\s*offsetof\s*\(\s*struct iwx_mcc_update_resp\s*,\s*channels\s*\)",
    )
    for index, pattern in enumerate(call_sites, 1):
        require(pattern, "packed response call site %d is missing its byte offset" % index)

    if re.search(
        r"iwx_init_channel_map\s*\([^;]*->\s*(?:regulatory\s*\.\s*channel_profile|channels)",
        source,
        re.S,
    ):
        failures.append("channel map call site still passes a packed member directly")
    if re.search(
        r"le(?:16|32)_to_cpup\s*\(\s*channel_profile_v[34]\s*\+\s*ch_idx",
        source,
    ):
        failures.append("channel profile still uses an unaligned typed load")
    return failures


def main():
    if len(sys.argv) != 2:
        print("usage: check-packed-member-alignment.py GENERATED_SOURCE", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    source = path.read_text(encoding="utf-8")
    failures = validate(source)
    if failures:
        for failure in failures:
            print("FAIL: " + failure, file=sys.stderr)
        return 1

    print("PASS: v3/v4 channel profiles use aligned locals and memcpy")
    print("PASS: all four packed response call sites use byte offsets")

    v4_copy = re.compile(
        r"memcpy\s*\(\s*&channel_flags_v4\s*,\s*channel_profile_v4\s*"
        r"\+\s*ch_idx\s*\*\s*sizeof\s*\(\s*channel_flags_v4\s*\)"
        r"\s*,\s*sizeof\s*\(\s*channel_flags_v4\s*\)\s*\)\s*;"
        r"\s*ch_flags\s*=\s*le32toh\s*\(\s*channel_flags_v4\s*\)\s*;",
        re.S,
    )
    sabotaged, count = v4_copy.subn(
        "ch_flags = le32_to_cpup((const uint32_t *)(channel_profile_v4 + "
        "ch_idx * sizeof (channel_flags_v4)));",
        source,
        count=1,
    )
    if count != 1:
        print("FAIL: could not construct the packed-member negative control", file=sys.stderr)
        return 1
    if not validate(sabotaged):
        print("FAIL: unaligned-load negative control was not detected", file=sys.stderr)
        return 1

    print("PASS: unaligned typed-load negative control rejected")
    return 0


if __name__ == "__main__":
    sys.exit(main())
