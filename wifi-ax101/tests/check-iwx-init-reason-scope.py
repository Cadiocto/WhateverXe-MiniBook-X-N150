#!/usr/bin/env python3
"""Validate file-scope visibility of all iwx init-task reason enumerators.

The enum was previously emitted after iwx_send_cmd() by scanfix patch 0028.
C enumerator identifiers are not visible before their declaration. This
checker verifies the generated C translation unit and runs controlled
negative controls for late, missing, and duplicate definitions.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys

EXPECTED = (
    "IWX_INIT_REASON_CMD_TIMEOUT",
    "IWX_INIT_REASON_BGSCAN_TASK",
    "IWX_INIT_REASON_NEWSTATE",
    "IWX_INIT_REASON_WATCHDOG",
    "IWX_INIT_REASON_CT_KILL",
    "IWX_INIT_REASON_CHANNEL_SWITCH",
    "IWX_INIT_REASON_RX_ERROR",
    "IWX_INIT_REASON_MSI_RF_KILL",
    "IWX_INIT_REASON_MSI_SW_ERROR",
    "IWX_INIT_REASON_MSI_HW_ERROR",
    "IWX_INIT_REASON_MSIX_TOP_FATAL",
    "IWX_INIT_REASON_MSIX_SW_ERROR",
    "IWX_INIT_REASON_MSIX_RF_KILL",
    "IWX_INIT_REASON_MSIX_HW_ERROR",
)


class ContractError(Exception):
    pass


def check_source(source: str) -> None:
    declarations = list(re.finditer(
        r"(?m)^enum\s+iwx_init_task_reason\s*\{", source))
    if len(declarations) != 1:
        raise ContractError(
            "expected exactly one file-scope enum iwx_init_task_reason; "
            "found %d" % len(declarations))

    start = declarations[0].start()
    close = source.find("};", declarations[0].end())
    if close < 0:
        raise ContractError("init-task reason enum is unterminated")
    block_end = close + 2
    block = source[declarations[0].end():close]
    defined = tuple(re.findall(
        r"(?m)^\s*(IWX_INIT_REASON_[A-Z0-9_]+)\s*(?:=[^,]+)?\s*,?\s*$",
        block))
    if defined != EXPECTED:
        raise ContractError(
            "init-task reason enum contents/order mismatch: %r" % (defined,))

    all_names = set(re.findall(r"\bIWX_INIT_REASON_[A-Z0-9_]+\b", source))
    missing = sorted(all_names - set(defined))
    if missing:
        raise ContractError("referenced init-task reasons are undeclared: " +
                            ", ".join(missing))

    function = re.search(
        r"(?m)^iwx_send_cmd\(struct iwx_softc \*sc, "
        r"struct iwx_host_cmd \*hcmd\)",
        source)
    if function is None:
        raise ContractError("cannot locate the iwx_send_cmd definition")
    first_use = re.search(
        r"iwx_schedule_init_task\(sc,\s*IWX_INIT_REASON_[A-Z0-9_]+\s*\)",
        source)
    if first_use is None:
        raise ContractError("no reason-tagged recovery call was found")
    if not (start < function.start() < first_use.start() < block_end):
        raise ContractError(
            "init-task reason enum must precede the first iwx_send_cmd use")
    if "static void\niwx_schedule_init_task(struct iwx_softc *sc, "
            "uint32_t reason)" not in source:
        raise ContractError(
            "iwx_schedule_init_task must retain its uint32_t reason argument")


def negative_controls(source: str) -> None:
    match = re.search(
        r"(?m)^enum\s+iwx_init_task_reason\s*\{.*?^\};\n",
        source, re.S)
    if match is None:
        raise ContractError("cannot isolate enum for negative-control tests")
    block = match.group(0)
    without = source[:match.start()] + source[match.end():]

    # Negative control 1: move the enum after the command timeout use.
    use = re.search(
        r"iwx_schedule_init_task\(sc, IWX_INIT_REASON_CMD_TIMEOUT\);",
        without)
    if use is None:
        raise ContractError("cannot find command-timeout negative-control site")
    late = without[:use.end()] + "\n\n" + block + without[use.end():]
    try:
        check_source(late)
    except ContractError:
        print("PASS: negative control rejected enum declared after first use")
    else:
        raise ContractError("late-enum negative control was not rejected")

    # Negative control 2: remove the command-timeout enumerator.
    missing = source.replace(
        "IWX_INIT_REASON_CMD_TIMEOUT = 1,",
        "IWX_INIT_REASON_TIMEOUT_REMOVED = 1,", 1)
    try:
        check_source(missing)
    except ContractError:
        print("PASS: negative control rejected missing CMD_TIMEOUT enumerator")
    else:
        raise ContractError("missing-enumerator negative control was not rejected")

    # Negative control 3: duplicate the enum definition.
    duplicate = source[:match.start()] + block + source[match.start():]
    try:
        check_source(duplicate)
    except ContractError:
        print("PASS: negative control rejected duplicate enum definition")
    else:
        raise ContractError("duplicate-enum negative control was not rejected")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("source", type=Path)
    args = parser.parse_args()
    try:
        source = args.source.read_text()
        check_source(source)
        print("PASS: all 14 IWX_INIT_REASON_* enumerators are defined exactly once")
        print("PASS: enum scope precedes iwx_send_cmd and all reason-tagged calls")
        if args.self_test:
            negative_controls(source)
            print("INIT_REASON_SCOPE_NEGATIVE_CONTROLS=3/3 PASS")
        print("INIT_REASON_SCOPE=PASS")
        return 0
    except (OSError, ContractError) as exc:
        print("INIT_REASON_SCOPE=FAIL: %s" % exc, file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
