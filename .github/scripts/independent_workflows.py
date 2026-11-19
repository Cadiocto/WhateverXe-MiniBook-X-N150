#!/usr/bin/env python3
"""Parse independent-workflow TSV evidence fail-closed."""
from __future__ import annotations
import json
import sys
from pathlib import Path

REQUIRED = ("callback_only", "ddi_integration")
STATUSES = {"queued", "in_progress", "completed", "waiting", "pending", "requested"}
CONCLUSIONS = {"success", "failure", "neutral", "cancelled", "skipped", "timed_out", "action_required", "stale", "startup_failure"}


class EvidenceError(ValueError):
    pass


def parse_text(text: str) -> dict[str, dict[str, object]]:
    records: dict[str, dict[str, object]] = {}
    lines = text.splitlines()
    if not lines:
        raise EvidenceError("evidence file is empty")
    for line_number, line in enumerate(lines, 1):
        if not line:
            raise EvidenceError(f"line {line_number}: blank rows are not allowed")
        fields = line.split("\t")
        if len(fields) != 4:
            raise EvidenceError(f"line {line_number}: expected 4 tab-separated fields (workflow, run_id, status, conclusion); found {len(fields)}")
        workflow, run_id, status, conclusion = fields
        if workflow not in REQUIRED:
            raise EvidenceError(f"line {line_number}: unexpected workflow {workflow!r}")
        if workflow in records:
            raise EvidenceError(f"line {line_number}: duplicate record for {workflow}")
        if not run_id.isascii() or not run_id.isdecimal() or int(run_id) <= 0:
            raise EvidenceError(f"line {line_number}: invalid run_id {run_id!r}")
        if status not in STATUSES:
            raise EvidenceError(f"line {line_number}: invalid status {status!r}")
        if conclusion and conclusion not in CONCLUSIONS:
            raise EvidenceError(f"line {line_number}: invalid conclusion {conclusion!r}")
        if status == "completed" and not conclusion:
            raise EvidenceError(f"line {line_number}: completed workflow has no conclusion")
        if status != "completed" and conclusion:
            raise EvidenceError(f"line {line_number}: non-completed workflow must have an empty conclusion")
        records[workflow] = {
            "run_id": run_id, "status": status, "conclusion": conclusion,
            "accepted": status == "completed" and conclusion == "success",
        }
    missing = [name for name in REQUIRED if name not in records]
    if missing:
        raise EvidenceError("missing required workflow record(s): " + ", ".join(missing))
    return records


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {Path(argv[0]).name} independent-workflows.tsv", file=sys.stderr)
        return 2
    try:
        parsed = parse_text(Path(argv[1]).read_text(encoding="utf-8"))
    except (OSError, UnicodeError, EvidenceError) as exc:
        print(f"INDEPENDENT_WORKFLOW_EVIDENCE_ERROR: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(parsed, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
