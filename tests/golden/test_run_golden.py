#!/usr/bin/env python3
"""Unit tests of the golden harness itself (run_golden.py): normalisation and comparison rules.

Stdlib only; run as `python3 -I tests/golden/test_run_golden.py` (also `meson test --suite golden`).
"""

import contextlib
import importlib.util
import io
import json
import re
import shutil
import struct
import tempfile
import unittest
import zipfile
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

    def test_profiles_independent_of_order(self):
        lines = SIGNALS.split("\n")
        swapped = "\n".join([lines[3], lines[1], lines[2], lines[0]])
        self.assertEqual(rg.id_profiles(rg.canonical_ids(swapped)), rg.id_profiles(rg.canonical_ids(SIGNALS)))
        # OUT's second print now shows the IN signal: same lines with IDs masked, other structure.
        other = SIGNALS.replace("t=1 s  OUT: [0015]", "t=1 s  OUT: [0012]")
        self.assertNotEqual(rg.id_profiles(other), rg.id_profiles(SIGNALS))


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

    def test_reordered_identical_prints(self):
        # Lines that only differ by their signal ID (same values, e.g. a testbench event log),
        # with whole time blocks swapped: ranks are permuted but the structure is the same.
        log = "\n".join(["0 s:", "OUT: [0003]  @ 1550 nm (1.0 W)", "IN: [0001]  @ 1550 nm (1.0 W)",
                         "1 s:", "OUT: [0007]  @ 1550 nm (1.0 W)", "IN: [0005]  @ 1550 nm (1.0 W)"])
        L = log.split("\n")
        swapped = "\n".join(L[3:] + L[:3])
        base, cand = rg.canonical_ids(log), rg.canonical_ids(swapped)
        self.assertTrue(rg.compare_case(stored(base), stored(cand), {}, DEFAULT_TOL)[0])
        diffs, notes = rg.compare_case(stored(base), stored(cand), {}, DEFAULT_TOL, lenient_order=True)
        self.assertEqual((diffs, len(notes)), ([], 1))
        # Same multiset of masked lines, but OUT at 1 s now repeats the 0 s signal: must fail.
        bad = rg.canonical_ids(swapped.replace("[0007]", "[0003]"))
        self.assertTrue(rg.compare_case(stored(base), stored(bad), {}, DEFAULT_TOL, lenient_order=True)[0])

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

    def test_gtest_absent_on_both_sides_fails(self):
        shutil.rmtree(self.base / "gtest")
        shutil.rmtree(self.cand / "gtest")
        out, rc = self.run_compare(allow_new=True)
        self.assertEqual((out["missing"], rc), ([rg.GTEST_ID], 1))
        self.assertEqual(self.run_compare("skip")[1], 0)

    def test_compare_no_unit_tests(self):
        shutil.rmtree(self.cand / "gtest")
        with contextlib.redirect_stdout(io.StringIO()):
            rc = rg.main(["compare", "--baseline", str(self.base), "--candidate", str(self.cand),
                          "--cases", str(self.cases_json()), "--no-unit-tests"])
            self.assertEqual(rc, 0)
            rc = rg.main(["compare", "--baseline", str(self.base), "--candidate", str(self.cand),
                          "--cases", str(self.cases_json())])
            self.assertEqual(rc, 1)

    def cases_json(self):
        p = Path(self.tmp.name) / "cases.json"
        p.write_text(json.dumps({"cases": self.CASES}), encoding="utf-8")
        return p

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


class PrintSlack(unittest.TestCase):
    """B-128: no print slack for tokens with fewer than PRINT_SLACK_MIN_DIGITS significant digits."""

    def same(self, a, b):
        return not rg.compare_text(a, b, DEFAULT_TOL, "stdout")

    def test_threshold(self):
        self.assertEqual(rg.PRINT_SLACK_MIN_DIGITS, 5)
        for tok, n in [("1e-08", 1), ("0.0001", 1), ("-0.8107", 4), ("0.000317", 3), ("0.00120", 3),
                       ("-2.6045", 5), ("1.000000", 7), ("0.000000", 0), ("1550", 4)]:
            self.assertEqual(rg.sig_digits(tok), n, tok)
        self.assertAlmostEqual(rg.printed_ulp("-2.6045"), 1e-4)
        self.assertAlmostEqual(rg.printed_ulp("0.707107"), 1e-6)
        self.assertAlmostEqual(rg.printed_ulp("6.73913e-07"), 1e-12)
        self.assertAlmostEqual(rg.printed_ulp("1.000000e-09"), 1e-15)

    def test_config_digit_changes_fail(self):
        for a, b in [("- abstol: 1e-08", "- abstol: 2e-08"), ("- abstol: 1e-08", "- abstol: 1e-07"),
                     ("- default reltol: 0.0001", "- default reltol: 0.0002"),
                     ("reltol (DOUBLE) = 1e-05,", "reltol (DOUBLE) = 2e-05,"),
                     ("resolution (DOUBLE) = 0.5,", "resolution (DOUBLE) = 0.6,"), ("pulse 2.0 W", "pulse 2.1 W"),
                     # below the default atol (1e-12): short tokens don't get it either
                     ("- abstol: 1e-12", "- abstol: 2e-12"), ("- abstol: 1e-12", "- abstol: 0")]:
            self.assertFalse(self.same(a, b), (a, b))

    def test_last_digit_of_long_tokens_still_tolerated(self):
        self.assertTrue(self.same("P = 0.707107 W", "P = 0.707108 W"))
        self.assertTrue(self.same("phi = -2.6045 rad", "phi = -2.6044 rad"))
        self.assertTrue(self.same("t = 6.73913e-07", "t = 6.73914e-07"))
        # Trailing zeros dropped by %g on one side: the longer token sets the slack.
        self.assertTrue(self.same("x = 0.12345", "x = 0.123451"))
        self.assertFalse(self.same("P = 0.707107 W", "P = 0.707109 W"))
        self.assertTrue(self.same("x = 0.1", "x = 0.0999999"))

    def test_mixed_lengths_use_the_longer_token(self):
        for a, b in [("1", "1.99999"), ("0", "0.99999"), ("0.1", "0.199999"), ("1550", "1550.9999"),
                     ("1e+10", "1.0002e+10")]:
            self.assertFalse(self.same("v = " + a, "v = " + b), (a, b))

    def test_short_tokens_use_rtol(self):
        self.assertTrue(self.same("-0.0000 rad", "0.0000 rad"))
        loose = rg.Tol(1e-3, 1e-12, 1.0)
        self.assertFalse(rg.compare_text("k = 0.5", "k = 0.5001", loose, "stdout"))
        self.assertTrue(rg.compare_text("k = 0.5", "k = 0.6", loose, "stdout"))


MANIFEST = """{
  "format": "specs-run-manifest",
  "format_version": 1,
  "specs_version": "%s",
  "systemc_version": "3.0.2-Accellera",
  "seed": 0,
  "command_line": ["specs", "-o", "traces/m", "-f", "a.cir"],
  "netlist_files": [
    {"path": "a.cir", "sha256": "%s"}
  ],
  "trace_file": "traces/m.vcd"
}
"""
HASH_A = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
HASH_B = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"


class RunManifest(unittest.TestCase):
    """F-01: the "specs_version" of the run manifest (file and --print-manifest) is masked."""

    def norm(self, text, pathmap=None):
        return rg.normalise_run_manifest(text, pathmap or rg.PathMap([]))

    def test_file_version_masked(self):
        out = self.norm(MANIFEST % ("v0.1-209.gd16e573", HASH_A))
        self.assertEqual(out, MANIFEST % ("<VERSION>", HASH_A))
        self.assertEqual(json.loads(out)["specs_version"], "<VERSION>")
        self.assertEqual(self.norm(out), out)

    def test_versions_compare_equal_other_changes_fail(self):
        a = self.norm(MANIFEST % ("v0.1-209.gd16e573", HASH_A))
        b = self.norm(MANIFEST % ("v0.2-1.g0123456-dirty", HASH_A))
        self.assertEqual(a, b)
        c = self.norm(MANIFEST % ("v0.2-1.g0123456-dirty", HASH_B))
        diffs = []
        rg.compare_json_values(json.loads(a), json.loads(c), DEFAULT_TOL, "m.manifest.json", diffs)
        self.assertEqual(len(diffs), 1, diffs)
        self.assertIn("sha256", diffs[0])

    def test_escaped_quotes_and_crlf(self):
        text = (MANIFEST % ('v1 \\"quoted\\" \\\\ x', HASH_A)).replace("\n", "\r\n")
        self.assertEqual(self.norm(text), MANIFEST % ("<VERSION>", HASH_A))

    def test_only_the_key_is_masked(self):
        # argv may contain the key as text: it is an escaped string value, not a key
        text = MANIFEST.replace('"-f", "a.cir"', '"-f", "\\"specs_version\\": \\"keep\\""') % ("v1", HASH_A)
        out = self.norm(text)
        self.assertEqual(json.loads(out)["command_line"][-1], '"specs_version": "keep"')
        self.assertEqual(json.loads(out)["specs_version"], "<VERSION>")

    def test_absolute_paths_mapped(self):
        text = MANIFEST.replace('["specs",', '["/opt/build/specs",') % ("v1", HASH_A)
        out = self.norm(text, rg.PathMap([("/opt/build", "<BUILD>")]))
        self.assertEqual(json.loads(out)["command_line"][0], "<BUILD>/specs")

    def test_print_manifest_stdout_line_masked(self):
        stdout = "Current SPECS config: \n" + MANIFEST % ("v0.1-209.gd16e573", HASH_A) + "DONE"
        out = rg.normalise_text(stdout, rg.PathMap([]))
        self.assertIn('\n  "specs_version": "<VERSION>",\n', out)
        self.assertNotIn("d16e573", out)
        self.assertIn(HASH_A, out)
        # elsewhere in a line, the text is not touched
        line = 'note: "specs_version": "v1"'
        self.assertEqual(rg.normalise_text(line, rg.PathMap([])), line)

    def test_run_case_masks_file_and_stdout(self):
        """End to end through run_case(), with a fake specs that writes and prints a manifest."""
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            fake = root / "bin" / "specs"
            fake.parent.mkdir()
            fake.write_text(
                "#!/bin/sh\n"
                "printf '{\\n  \"specs_version\": \"%s\",\\n  \"command_line\": [\"%s/specs\"]\\n}\\n' "
                "\"$1\" \"$PWD\" > traces/m.manifest.json\n"
                "printf '{\\n  \"specs_version\": \"%s\"\\n}\\n' \"$1\"\n"
            )
            fake.chmod(0o755)
            (root / "repo").mkdir()
            (root / "tmp").mkdir()
            case = {"id": "x", "setup": {"mkdir": ["traces"]}}
            res = [rg.run_case(dict(case, args=[v]), fake, root / "repo", str(root / "tmp"))
                   for v in ("v0.1-209.gd16e573", "v0.2")]
            for r in res:
                self.assertEqual(r["exit"], 0, r["stderr"])
                content = r["files"]["traces/m.manifest.json"]["content"]
                self.assertEqual(r["files"]["traces/m.manifest.json"]["kind"], "json")
                self.assertEqual(json.loads(content),
                                 {"specs_version": "<VERSION>", "command_line": ["<CWD>/specs"]})
                self.assertEqual(r["stdout"], '{\n  "specs_version": "<VERSION>"\n}\n')
            self.assertEqual(res[0]["files"], res[1]["files"])



STATS = """{
  "format": "specs-run-stats",
  "format_version": 1,
  "end_time_s": 3e-09,
  "wall_time_s": %s,
  "totals": {"output_ports": 2, "scheduled": %d, "pending_mean": 1.5}
}
"""


class RunStats(unittest.TestCase):
    """F-15: "wall_time_s" of the run statistics (file and --print-stats) is masked, nothing else."""

    def test_file_wall_time_masked(self):
        out = rg.normalise_run_stats(STATS % ("0.004330671", 4))
        self.assertEqual(out, STATS % ('"<WALL_TIME>"', 4))
        self.assertEqual(json.loads(out)["wall_time_s"], "<WALL_TIME>")
        self.assertEqual(rg.normalise_run_stats(out), out)
        for t in ("1.25e-05", "12", "0"):
            self.assertEqual(rg.normalise_run_stats(STATS % (t, 4)), out)

    def test_counters_still_compared(self):
        a = json.loads(rg.normalise_run_stats(STATS % ("0.1", 4)))
        b = json.loads(rg.normalise_run_stats(STATS % ("0.2", 5)))
        diffs = []
        rg.compare_json_values(a, b, DEFAULT_TOL, "x.stats.json", diffs)
        self.assertEqual(len(diffs), 1, diffs)
        self.assertIn("scheduled", diffs[0])

    def test_print_stats_stdout_line_masked(self):
        out = rg.normalise_text("Simulated 3 ns\n" + STATS % ("0.5", 4) + "DONE", rg.PathMap([]))
        self.assertIn('\n  "wall_time_s": "<WALL_TIME>",\n', out)
        self.assertIn('"end_time_s": 3e-09', out)
        # elsewhere in a line, the text is not touched
        line = 'note: "wall_time_s": 0.5'
        self.assertEqual(rg.normalise_text(line, rg.PathMap([])), line)

    def test_run_case_masks_the_file(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            fake = root / "bin" / "specs"
            fake.parent.mkdir()
            fake.write_text(
                "#!/bin/sh\n"
                "printf '{\\n  \"wall_time_s\": %s,\\n  \"processed\": 3\\n}\\n' \"$1\" > traces/m.stats.json\n"
            )
            fake.chmod(0o755)
            (root / "repo").mkdir()
            (root / "tmp").mkdir()
            case = {"id": "x", "setup": {"mkdir": ["traces"]}}
            res = [rg.run_case(dict(case, args=[v]), fake, root / "repo", str(root / "tmp"))
                   for v in ("0.001", "2.5")]
            for r in res:
                self.assertEqual(r["exit"], 0, r["stderr"])
                f = r["files"]["traces/m.stats.json"]
                self.assertEqual(f["kind"], "json")
                self.assertEqual(json.loads(f["content"]), {"wall_time_s": "<WALL_TIME>", "processed": 3})
            self.assertEqual(res[0]["files"], res[1]["files"])


# A fake specs for the stats commands: writes the file given to --stats-file (if any) with the wall
# time of the run and "processed" = $PROCESSED (default 3), unless the first argument is "-n"
FAKE_STATS_SPECS = (
    "#!/bin/sh\n"
    "[ \"$1\" = -n ] && exit 0\n"
    "out=\"\"\n"
    "while [ $# -gt 0 ]; do [ \"$1\" = --stats-file ] && out=\"$2\"; shift; done\n"
    "[ -n \"$out\" ] || exit 1\n"
    "printf '{\\n  \"format\": \"specs-run-stats\",\\n  \"wall_time_s\": %s,\\n  "
    "\"totals\": {\"processed\": %s},\\n  \"ports\": [\\n    {\"port\": \"a.w\", \"processed\": %s},\\n"
    "    {\"port\": \"b.w\", \"processed\": 1}\\n  ]\\n}\\n' \"$(date +%N)\" \"${PROCESSED:-3}\" "
    "\"${PROCESSED:-3}\" > \"$out\"\n"
)


class Stats(unittest.TestCase):
    """F-15: stats-record runs every case with --stats, stats-compare compares the counters exactly."""

    def setUp(self):
        self.td = tempfile.TemporaryDirectory()
        self.root = Path(self.td.name)
        self.fake = self.root / "bin" / "specs"
        self.fake.parent.mkdir()
        self.fake.write_text(FAKE_STATS_SPECS)
        self.fake.chmod(0o755)
        (self.root / "repo").mkdir()
        (self.root / "tmp").mkdir()
        self.cases = self.root / "cases.json"
        self.cases.write_text(json.dumps({"cases": [
            {"id": "a/run", "args": ["-f", "x.cir"]},
            {"id": "b/dry", "args": ["-n"]},
        ]}))

    def tearDown(self):
        self.td.cleanup()

    def record(self, name, processed=None):
        out = self.root / name
        argv = ["stats-record", "--cases", str(self.cases), "--specs", str(self.fake), "--out", str(out),
                "--repo", str(self.root / "repo"), "--tmpdir", str(self.root / "tmp")]
        env = dict(rg.os.environ)
        try:
            if processed is not None:
                rg.os.environ["PROCESSED"] = str(processed)
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(rg.main(argv), 0)
        finally:
            rg.os.environ.clear()
            rg.os.environ.update(env)
        return out

    def compare(self, a, b, *extra):
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = rg.main(["stats-compare", "--cases", str(self.cases), "--baseline", str(a),
                          "--candidate", str(b)] + list(extra))
        return rc, buf.getvalue()

    def test_stats_case_appends_the_options(self):
        c = {"id": "x", "args": ["-f", "a.cir"]}
        self.assertEqual(rg.stats_case(c)["args"], ["-f", "a.cir"] + rg.STATS_ARGS)
        self.assertEqual(c["args"], ["-f", "a.cir"])
        self.assertIn("--stats=ports,devices", rg.STATS_ARGS)

    def test_record_drops_the_wall_time(self):
        out = self.record("a")
        res = json.loads((out / "cases" / "a" / "run.json").read_text())
        self.assertEqual(res["exit"], 0)
        self.assertNotIn("wall_time_s", res["stats"])
        self.assertEqual(res["stats"]["totals"], {"processed": 3})
        dry = json.loads((out / "cases" / "b" / "dry.json").read_text())
        self.assertIsNone(dry["stats"])
        manifest = json.loads((out / "manifest.json").read_text())
        self.assertEqual(manifest["format"], rg.STATS_FORMAT)
        self.assertEqual(manifest["cases"], ["a/run", "b/dry"])

    def test_same_build_compares_equal(self):
        rc, text = self.compare(self.record("a"), self.record("b"))
        self.assertEqual(rc, 0, text)
        self.assertIn("2 same (1 of them without run statistics), 0 differ", text)

    def test_counter_change_fails_and_names_the_row(self):
        rc, text = self.compare(self.record("a"), self.record("b", processed=4))
        self.assertEqual(rc, 1, text)
        self.assertIn("DIFF     a/run", text)
        self.assertIn("totals.processed: 3 -> 4", text)
        self.assertIn("ports.a.w.processed: 3 -> 4", text)
        self.assertNotIn("b.w", text)
        rc, text = self.compare(self.record("c"), self.record("d", processed=4), "--ignore", "processed")
        self.assertEqual(rc, 0, text)

    def test_missing_result_fails(self):
        a, b = self.record("a"), self.record("b")
        (b / "cases" / "a" / "run.json").unlink()
        rc, text = self.compare(a, b)
        self.assertEqual(rc, 1)
        self.assertIn("MISSING  a/run", text)

    def test_compare_stats_exact(self):
        a = {"exit": 0, "stats": {"totals": {"pending_mean": 1.5, "x": True}, "ports": [{"port": "p", "n": 1}]}}
        self.assertEqual(rg.compare_stats(a, json.loads(json.dumps(a))), [])
        b = json.loads(json.dumps(a))
        b["stats"]["totals"]["pending_mean"] = 1.5000000000000002
        b["stats"]["totals"]["x"] = 1
        b["exit"] = 1
        diffs = rg.compare_stats(a, b)
        self.assertEqual(diffs, ["exit: 0 -> 1", "totals.pending_mean: 1.5 -> 1.5000000000000002",
                                 "totals.x: true -> 1"])
        c = json.loads(json.dumps(a))
        c["stats"]["ports"].append({"port": "q", "n": 2})
        self.assertEqual(rg.compare_stats(a, c), ['ports.q: (absent) -> {"port": "q", "n": 2}'])
        self.assertEqual(rg.compare_stats(a, {"exit": 0, "stats": None})[0][:10], "stats: {\"t")


def make_npy(descr, shape, payload, version=1):
    """An NPY file; the header is padded so that the data start at a multiple of 64 bytes, as numpy does."""
    header = "{'descr': '%s', 'fortran_order': False, 'shape': %r, }" % (descr, shape)
    pre = 10 if version == 1 else 12
    pad = (-(pre + len(header) + 1)) % 64
    header = (header + " " * pad + "\n").encode("ascii")
    if version == 1:
        head = rg.NPY_MAGIC + b"\x01\x00" + struct.pack("<H", len(header))
    else:
        head = rg.NPY_MAGIC + b"\x02\x00" + struct.pack("<I", len(header))
    return head + header + payload


def f8(*vals):
    return make_npy("<f8", (len(vals),), struct.pack("<%dd" % len(vals), *vals))


def make_npz(entries):
    """entries: list of (name, npy bytes); STORED, in the given order."""
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_STORED) as zf:
        for name, data in entries:
            zf.writestr(zipfile.ZipInfo(name), data)
    return buf.getvalue()


def npz_result(raw, name="traces/x.npz"):
    """A run_case()-style result holding one decoded .npz file."""
    return {"id": "npz/x", "args": [], "exit": 0, "stdout": "", "stderr": "",
            "files": {name: {"kind": "npz", "content": rg.decode_npz(raw),
                             "raw_sha256": rg.sha256(raw), "raw_bytes": len(raw)}}}


def npz_stored(raw):
    res = npz_result(raw)
    f = res["files"]["traces/x.npz"]
    data = f["content"].encode("utf-8")
    return {"args": [], "exit": 0, "stdout": "", "stderr": "",
            "files": {"traces/x.npz": {"kind": "npz", "content": f["content"], "bytes": len(data),
                                       "sha256": rg.sha256(data)}}}


class Npz(unittest.TestCase):
    """.npz outputs are decoded to text and compared with the numeric-aware text comparison."""

    def test_file_kind(self):
        self.assertEqual(rg.file_kind("traces/x.NPZ"), "npz")
        self.assertEqual(rg.file_kind("traces/x.npz"), "npz")

    def test_decode_types_and_order(self):
        strs = ["a", "wl1"]
        raw = make_npz([
            ("time_s.npy", f8(0.0, 0.1, float("nan"), float("inf"), -float("inf"), 1e-16, -2.5)),
            ("signal.npy", make_npy("<i4", (3,), struct.pack("<3i", 0, -1, 2147483647))),
            ("big.npy", make_npy("<i8", (2,), struct.pack("<2q", 2**40, -3))),
            ("signal_names.npy",
             make_npy("<U5", (2,), b"".join(x.encode("utf-32-le").ljust(20, b"\0") for x in strs))),
            ("m.npy", make_npy("<f8", (2, 2), struct.pack("<4d", 1, 2, 3, 4.5), version=2)),
            ("scalar.npy", make_npy("<i4", (), struct.pack("<i", 7))),
            ("empty.npy", make_npy("<f8", (0,), b"")),
        ])
        expect = "\n".join([
            "npz time_s.npy: descr=<f8 shape=(7,)", "0.0", "0.1", "nan", "inf", "-inf", "1e-16", "-2.5",
            "npz signal.npy: descr=<i4 shape=(3,)", "0", "-1", "2147483647",
            "npz big.npy: descr=<i8 shape=(2,)", str(2**40), "-3",
            "npz signal_names.npy: descr=<U5 shape=(2,)", "a", "wl1",
            "npz m.npy: descr=<f8 shape=(2, 2)", "1.0", "2.0", "3.0", "4.5",
            "npz scalar.npy: descr=<i4 shape=()", "7",
            "npz empty.npy: descr=<f8 shape=(0,)",
        ]) + "\n"
        self.assertEqual(rg.decode_npz(raw), expect)

    def test_shortest_roundtrip_repr(self):
        x = 0.1 + 0.2
        out = rg.decode_npz(make_npz([("a.npy", f8(x))]))
        self.assertEqual(out.split("\n")[1], "0.30000000000000004")

    def test_ulp_difference_equal_but_1e6_differs(self):
        x = 1.2345678901234567e-3
        ulp = struct.unpack("<d", struct.pack("<Q", struct.unpack("<Q", struct.pack("<d", x))[0] + 1))[0]
        self.assertNotEqual(ulp, x)
        case = {"id": "npz/x"}
        a = npz_stored(make_npz([("a.npy", f8(x, 3.0))]))
        b = npz_stored(make_npz([("a.npy", f8(ulp, 3.0))]))
        self.assertNotEqual(a["files"]["traces/x.npz"]["sha256"], b["files"]["traces/x.npz"]["sha256"])
        diffs, _ = rg.compare_case(a, b, case, DEFAULT_TOL)
        self.assertEqual(diffs, [])
        c = npz_stored(make_npz([("a.npy", f8(x * (1 + 1e-6), 3.0))]))
        diffs, _ = rg.compare_case(a, c, case, DEFAULT_TOL)
        self.assertEqual(len(diffs), 1, diffs)
        self.assertIn("traces/x.npz", diffs[0])
        # a different shape or entry name is a diff too
        d = npz_stored(make_npz([("a.npy", f8(x, 3.0, 4.0))]))
        self.assertTrue(rg.compare_case(a, d, case, DEFAULT_TOL)[0])
        e = npz_stored(make_npz([("b.npy", f8(x, 3.0))]))
        self.assertTrue(rg.compare_case(a, e, case, DEFAULT_TOL)[0])

    def test_undecodable_never_raises(self):
        self.assertEqual(rg.decode_npz(b"not a zip"), "npz: cannot decode: not a zip archive\n")
        self.assertEqual(rg.decode_npz(b""), "npz: cannot decode: not a zip archive\n")
        good = f8(1.0, 2.0)
        cases = {
            "bad magic": b"\x92NUMPY" + good[6:],
            "unsupported descr": make_npy(">f4", (1,), struct.pack(">f", 1.0)),
            "fortran": good.replace(b"'fortran_order': False", b"'fortran_order': True "),
            "truncated data": good[:-3],
            "trailing data": good + b"\0",
            "truncated header": good[:20],
            "version 3": good[:6] + b"\x03\x00" + good[8:],
            "empty": b"",
        }
        for what, npy in cases.items():
            out = rg.decode_npz(make_npz([("a.npy", f8(5.0)), ("bad.npy", npy), ("c.npy", f8(6.0))]))
            lines = out.split("\n")
            self.assertEqual(lines[:2], ["npz a.npy: descr=<f8 shape=(1,)", "5.0"], what)
            self.assertTrue(lines[2].startswith("npz bad.npy: cannot decode: "), (what, lines[2]))
            self.assertEqual(lines[3:], ["npz c.npy: descr=<f8 shape=(1,)", "6.0", ""], what)
        self.assertIn("unsupported descr", rg.decode_npz(make_npz([("a.npy", cases["unsupported descr"])])))

    def test_store_load_roundtrip(self):
        raw = make_npz([("time_s.npy", f8(0.0, 1e-9)), ("names.npy", make_npy(
            "<U3", (1,), "ab".encode("utf-32-le").ljust(12, b"\0")))])
        res = npz_result(raw)
        with tempfile.TemporaryDirectory() as td:
            out = Path(td)
            rg.store_result(out, res, 10**6)
            cdir = out / "cases" / rg.safe_rel("npz/x")
            self.assertTrue((cdir / "files" / "traces" / "x.npz.txt").is_file())
            self.assertFalse((cdir / "files" / "traces" / "x.npz").exists())
            meta = json.loads((cdir / "case.json").read_text(encoding="utf-8"))
            entry = meta["files"]["traces/x.npz"]
            self.assertEqual(entry["kind"], "npz")
            self.assertEqual(entry["stored"], "files/traces/x.npz.txt")
            self.assertEqual(entry["raw_sha256"], rg.sha256(raw))
            self.assertEqual(entry["raw_bytes"], len(raw))
            self.assertEqual(entry["sha256"], rg.sha256(res["files"]["traces/x.npz"]["content"]))
            back = rg.load_stored(out, "npz/x")
            self.assertEqual(back["files"]["traces/x.npz"]["content"], res["files"]["traces/x.npz"]["content"])
            self.assertEqual(rg.compare_case(back, back, {"id": "npz/x"}, DEFAULT_TOL), ([], []))
            # too large: gzipped (still text), or only a line count
            rg.store_result(out, res, 40)
            entry = json.loads((cdir / "case.json").read_text(encoding="utf-8"))["files"]["traces/x.npz"]
            self.assertTrue(entry["stored"] is None or entry["stored"] == "files/traces/x.npz.txt.gz")
            if entry["stored"] is None:
                self.assertIn("lines", entry["summary"])
            back = rg.load_stored(out, "npz/x")
            if entry["stored"]:
                self.assertEqual(back["files"]["traces/x.npz"]["content"],
                                 res["files"]["traces/x.npz"]["content"])

    def test_run_case_decodes_the_file(self):
        raw = make_npz([("a.npy", f8(1.0, 2.0))])
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "repo").mkdir()
            (root / "tmp").mkdir()
            (root / "repo" / "fixture.npz").write_bytes(raw)
            fake = root / "specs"
            fake.write_text("#!/bin/sh\ncp fixture.npz traces/out.npz\n")
            fake.chmod(0o755)
            case = {"id": "x", "setup": {"copy": ["fixture.npz"], "mkdir": ["traces"]}}
            res = rg.run_case(case, fake, root / "repo", str(root / "tmp"))
            self.assertEqual(res["exit"], 0, res["stderr"])
            f = res["files"]["traces/out.npz"]
            self.assertEqual(f["kind"], "npz")
            self.assertEqual(f["content"], rg.decode_npz(raw))
            self.assertEqual((f["raw_sha256"], f["raw_bytes"]), (rg.sha256(raw), len(raw)))


if __name__ == "__main__":
    unittest.main(verbosity=2)
