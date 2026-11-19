#!/usr/bin/env python3
"""Regression tests for fail-closed independent-workflow evidence parsing."""
import importlib.util
import unittest
from pathlib import Path

MODULE_PATH = Path(__file__).resolve().parents[1] / "scripts" / "independent_workflows.py"
SPEC = importlib.util.spec_from_file_location("independent_workflows", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class IndependentWorkflowParserTests(unittest.TestCase):
    def test_real_run_44605375271_four_column_tsv_regression(self):
        parsed = MODULE.parse_text(
            "callback_only\t44605375129\tcompleted\tsuccess\n"
            "ddi_integration\t44605375289\tcompleted\tsuccess\n"
        )
        self.assertEqual(parsed["callback_only"], {"run_id":"44605375129","status":"completed","conclusion":"success","accepted":True})
        self.assertEqual(parsed["ddi_integration"], {"run_id":"44605375289","status":"completed","conclusion":"success","accepted":True})

    def test_completed_failure_is_not_accepted(self):
        parsed = MODULE.parse_text("callback_only\t101\tcompleted\tfailure\nddi_integration\t102\tcompleted\tsuccess\n")
        self.assertFalse(parsed["callback_only"]["accepted"])

    def test_in_progress_is_not_accepted(self):
        parsed = MODULE.parse_text("callback_only\t101\tin_progress\t\nddi_integration\t102\tcompleted\tsuccess\n")
        self.assertFalse(parsed["callback_only"]["accepted"])

    def test_missing_workflow_fails(self):
        with self.assertRaisesRegex(MODULE.EvidenceError, "missing required"):
            MODULE.parse_text("callback_only\t101\tcompleted\tsuccess\n")

    def test_blank_row_fails(self):
        with self.assertRaisesRegex(MODULE.EvidenceError, "blank rows"):
            MODULE.parse_text("callback_only\t101\tcompleted\tsuccess\n\nddi_integration\t102\tcompleted\tsuccess\n")

    def test_damaged_row_fails(self):
        with self.assertRaisesRegex(MODULE.EvidenceError, "expected 4"):
            MODULE.parse_text("callback_only\t101\tcompleted\nddi_integration\t102\tcompleted\tsuccess\n")

    def test_duplicate_record_fails(self):
        with self.assertRaisesRegex(MODULE.EvidenceError, "duplicate record"):
            MODULE.parse_text("callback_only\t101\tcompleted\tsuccess\ncallback_only\t101\tcompleted\tsuccess\nddi_integration\t102\tcompleted\tsuccess\n")

    def test_completed_without_conclusion_fails(self):
        with self.assertRaisesRegex(MODULE.EvidenceError, "no conclusion"):
            MODULE.parse_text("callback_only\t101\tcompleted\t\nddi_integration\t102\tcompleted\tsuccess\n")

    def test_noncompleted_with_conclusion_fails(self):
        with self.assertRaisesRegex(MODULE.EvidenceError, "non-completed"):
            MODULE.parse_text("callback_only\t101\tin_progress\tsuccess\nddi_integration\t102\tcompleted\tsuccess\n")


if __name__ == "__main__":
    unittest.main(verbosity=2)
