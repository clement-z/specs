# Phase L — B-186: MLPROBE records each `.dc` point (branch `ws/mlprobedc`)

Follow `COMMON.md`. Phase id `PL`. Decision: `backlog/DECISIONS-2026-10-21.md`, row "B-186 and B-188" (2026-10-24).
Read `backlog/B-186-*.md` (problem, evidence, the update after phase H), the PD5 and PH reports
(`backlog/sprint-2026-10c/PD5-REPORT.md`, `PH-REPORT.md`: PROBE latch, fresh `.dc` points), `src/devices/probe.cpp`,
the `.dc` runner in `src/specs.cpp`, and `doc/netlist.md` (PROBE, MLPROBE, output formats). Another phase (K) runs
in parallel on parser, sources and testbenches: stay within this item. Write your predictions (golden cases,
counters) in `PL-progress.md` before each re-record.

## Scope (maintainer's decisions)
1. MLPROBE is enabled in `.dc`: at each point it records the converged value of each listed wavelength, in the
   VCD and in the CSV/npz tables (one row per point, probe and wavelength, as the tables already allow).
2. A listed wavelength that the net does not carry at a point records **0** (each `.dc` point is a fresh steady
   state, phase H).
3. Roles unchanged: in a `.dc` wavelength or frequency sweep, PROBE keeps following the swept wavelength (D5
   latch); MLPROBE records its fixed listed wavelengths (0 where the net does not carry them at that point).
4. `.op`: check whether MLPROBE records there; if it is disabled by the same switch, include it with the same
   rules and say so; if it is a separate question, leave it and report.
5. Tests and docs: the repro `backlog/repro/b186-mlprobe-dc-wavelength-sweep.cir` as a golden case; a case with a
   non-wavelength sweep of a WDM circuit (e.g. a source power or a parameter swept, two or more channels);
   `doc/netlist.md` (MLPROBE in `.dc`), CHANGELOG `[unreleased]` (Fixed), `backlog/B-186-*.md` status and its row
   in `backlog.md`.

## Acceptance
- `pixi run test gcc`, `pixi run test asan-clang`: all OK, no sanitizer message.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: only predicted cases change (existing `.dc` cases
  with an MLPROBE gain values; nothing else), each as predicted.
- clang and asan-gcc: before/after `compare` (strict) shows the same set.
- `stats-compare` (gcc) before/after: identical except predicted cases (explain any probe counter change).
- Report `backlog/sprint-2026-10c/PL-REPORT.md` committed on `ws/mlprobedc`: the completion marker.
