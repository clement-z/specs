#!/usr/bin/env python3
"""Unit tests of the golden harness itself (run_golden.py): normalisation and comparison rules.

Stdlib only; run as `python3 -I tests/golden/test_run_golden.py` (also `meson test --suite golden`).
"""

import contextlib
import importlib.util
import io
import re
import shutil
import tempfile
import unittest
from pathlib import Path

_spec = importlib.util.spec_from_file_location("run_golden", Path(__file__).resolve().parent / "run_golden.py")
rg = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(rg)

DEFAULT_TOL = rg.Tol(1e-9, 1e-12, 1.0)


def stored(stdout, stderr="", exit=0):
    """Minimal 'stored' dict as returned by load_stored()."""
    return {"args": [], "exit": exit, "stdout": stdout, "stderr": stderr, "files": {}}


SIGNALS = "\n".join([
    "t=0 s  OUT: [0015]  @ 1550 nm (1.000000 √W, 1.000000 W, 0.0000 rad)",
    "t=0 s  IN:  [0012]  @ 1550 nm (1.000000 √W, 1.000000 W, 0.0000 rad)",
    "t=1 s  OUT: [0015]  @ 1550 nm (1.000000 √W, 1.000000 W, 0.0000 rad)",
    "t=1 s  X:   [0042]  @ 1550 nm (0.500000 √W, 0.250000 W, 0.0000 rad)",
])


def shift_ids(text, k):
    return re.sub(r"\[(\d{4,})\](?= +@ )", lambda m: "[%04d]" % (int(m.group(1)) + k), text)


class SignalIds(unittest.TestCase):
    """B-129: printed OpticalSignal IDs become first-appearance ranks."""

    def test_ranks(self):
        out = rg.canonical_ids(SIGNALS).split("\n")
        self.assertIn("OUT: [#1]  @", out[0])
        self.assertIn("IN:  [#2]  @", out[1])
        self.assertIn("OUT: [#1]  @", out[2])
        self.assertIn("X:   [#3]  @", out[3])

    def test_normalise_text_applies_it(self):
        self.assertEqual(rg.normalise_text(SIGNALS, rg.PathMap([])), rg.canonical_ids(SIGNALS))

    def test_idempotent(self):
        once = rg.canonical_ids(SIGNALS)
        self.assertEqual(rg.canonical_ids(once), once)

    def test_offset_vanishes(self):
        self.assertEqual(rg.canonical_ids(shift_ids(SIGNALS, 7)), rg.canonical_ids(SIGNALS))

    def test_identity_structure_kept(self):
        # The last print refers to the first signal instead of a new one: must differ.
        other = SIGNALS.replace("[0042]", "[0015]")
        self.assertNotEqual(rg.canonical_ids(other), rg.canonical_ids(SIGNALS))
        diffs, _ = rg.compare_case(stored(rg.canonical_ids(SIGNALS)), stored(rg.canonical_ids(other)), {}, DEFAULT_TOL)
        self.assertTrue(diffs)

    def test_other_brackets_untouched(self):
        text = "matrix [1234] [0015] x\n[0015]  @ 1550 nm"
        self.assertEqual(rg.canonical_ids(text), "matrix [1234] [0015] x\n[#1]  @ 1550 nm")

    def test_sort_lines_independent_of_order(self):
        lines = SIGNALS.split("\n")
        swapped = "\n".join([lines[3], lines[1], lines[2], lines[0]])
        a = sorted(rg.canonical_ids(SIGNALS, sort_lines=True).split("\n"))
        b = sorted(rg.canonical_ids(rg.canonical_ids(swapped), sort_lines=True).split("\n"))
        self.assertEqual(a, b)


class LineOrder(unittest.TestCase):
    """B-126: line order matters, except in unordered_streams or with lenient_order."""

    A = "t=0 s: a = 1\nt=0 s: b = 2\nt=1 s: a = 3\nt=1 s: b = 4"
    B = "t=1 s: a = 3\nt=1 s: b = 4\nt=0 s: a = 1\nt=0 s: b = 2"

    def test_reorder_fails_by_default(self):
        diffs, _ = rg.compare_case(stored(self.A), stored(self.B), {}, DEFAULT_TOL)
        self.assertTrue(diffs)
        self.assertIn("ONLY THE LINE ORDER DIFFERS", diffs[0])

    def test_unordered_stream_passes_with_note(self):
        diffs, notes = rg.compare_case(stored(self.A), stored(self.B), {"unordered_streams": ["stdout"]}, DEFAULT_TOL)
        self.assertEqual(diffs, [])
        self.assertEqual(notes, ["stdout: line order differs (reordered)"])

    def test_unordered_is_per_stream(self):
        diffs, _ = rg.compare_case(stored("", self.A), stored("", self.B), {"unordered_streams": ["stdout"]}, DEFAULT_TOL)
        self.assertTrue(diffs)

    def test_lenient_order(self):
        diffs, notes = rg.compare_case(stored(self.A), stored(self.B), {}, DEFAULT_TOL, lenient_order=True)
        self.assertEqual(diffs, [])
        self.assertTrue(notes)

    def test_other_differences_still_fail(self):
        diffs, _ = rg.compare_case(stored(self.A), stored(self.B.replace("= 4", "= 5")),
                                   {"unordered_streams": ["stdout"]}, DEFAULT_TOL)
        self.assertTrue(diffs)

    def test_reordered_signal_prints(self):
        # The first print of a signal moves: its rank changes, the multiset comparison must not care.
        lines = SIGNALS.split("\n")
        moved = "\n".join([lines[3], lines[0], lines[1], lines[2]])
        base, cand = rg.canonical_ids(SIGNALS), rg.canonical_ids(moved)
        self.assertNotEqual(sorted(base.split("\n")), sorted(cand.split("\n")))
        diffs, _ = rg.compare_case(stored(base), stored(cand), {"unordered_streams": ["stdout"]}, DEFAULT_TOL)
        self.assertEqual(diffs, [])


class Coverage(unittest.TestCase):
    """B-127: NEW, MISSING and an unrequested gtest skip fail; --only must select something."""

    CASES = [{"id": "cir/a"}, {"id": "cir/b"}]

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        root = Path(self.tmp.name)
        self.base, self.cand = root / "base", root / "cand"
        for d in (self.base, self.cand):
            for c in self.CASES:
                rg.store_result(d, {"id": c["id"], "args": [], "exit": 0, "stdout": "x 1.5", "stderr": "",
                                    "files": {}}, 10**6)
            rg.store_gtest(d, {"id": rg.GTEST_ID, "exit": 0, "tests": {"S.T": "PASS"}})

    def tearDown(self):
        self.tmp.cleanup()

    def run_compare(self, gtest="compare", allow_new=False):
        with contextlib.redirect_stdout(io.StringIO()):
            out = rg.compare_dirs(self.base, self.cand, self.CASES, DEFAULT_TOL, gtest)
            return out, rg.summarise(out, allow_new)

    def test_all_present_passes(self):
        out, rc = self.run_compare()
        self.assertEqual((rc, len(out["passed"])), (0, 3))

    def test_new_case_fails_unless_allowed(self):
        shutil.rmtree(self.base / "cases" / "cir" / "b")
        out, rc = self.run_compare()
        self.assertEqual((out["new"], rc), (["cir/b"], 1))
        self.assertEqual(self.run_compare(allow_new=True)[1], 0)

    def test_missing_case_fails(self):
        shutil.rmtree(self.cand / "cases" / "cir" / "b")
        out, rc = self.run_compare(allow_new=True)
        self.assertEqual((out["missing"], rc), (["cir/b"], 1))

    def test_gtest_not_run_fails(self):
        shutil.rmtree(self.cand / "gtest")
        out, rc = self.run_compare(allow_new=True)
        self.assertEqual((out["missing"], rc), ([rg.GTEST_ID], 1))

    def test_gtest_skipped_explicitly(self):
        shutil.rmtree(self.cand / "gtest")
        out, rc = self.run_compare("skip")
        self.assertEqual((out["skipped"], rc), ([rg.GTEST_ID], 0))

    def test_gtest_not_selected(self):
        shutil.rmtree(self.cand / "gtest")
        out, rc = self.run_compare("off")
        self.assertEqual((out["missing"], out["skipped"], rc), ([], [], 0))

    def test_new_gtest_fails_unless_allowed(self):
        shutil.rmtree(self.base / "gtest")
        self.assertEqual(self.run_compare()[1], 1)
        self.assertEqual(self.run_compare(allow_new=True)[1], 0)

    def test_select(self):
        self.assertEqual(rg.select(self.CASES, None), (self.CASES, True))
        self.assertEqual(rg.select(self.CASES, "a$"), ([self.CASES[0]], False))
        self.assertEqual(rg.select(self.CASES, "^gtest"), ([], True))

    def test_select_nothing_exits_2(self):
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as cm:
            rg.select(self.CASES, "no_such_case")
        self.assertEqual(cm.exception.code, 2)


if __name__ == "__main__":
    unittest.main(verbosity=2)
