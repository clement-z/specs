# Phase G — cleanup after v0.3.0 (branch `ws/cleanup3`)

Follow `COMMON.md` (heartbeat, progress file, commits, report, never push) and the gate of `PD-common-f15.md`
(pixi builds, strict gcc golden, clang/asan-gcc before/after, counters). Phase id `PG`. Maintainer go: 2026-10-23.
You are Opus at high effort; use Sonnet subagents (effort medium) for the bookkeeping of item 1 and review their
diffs. One commit per item; predicted golden impact written in `PG-progress.md` before any output change.

1. **Backlog statuses.** Many open items in `backlog.md` predate sprint 2026-10c phases B-183, C2, D1–D6 (reports in
   `backlog/sprint-2026-10c/`). Check each open item against the current code and those reports; update the
   ones that are fixed, partly fixed or obsolete (status line of the file + `backlog.md` row, with the commit and a
   one-line reason). Known candidates: B-057 (PROBE on WDM nets: D5 latch), B-108 (golden bit-exactness:
   B-183), B-015 (per-event cost: D2), B-078 (pyspecs, wavelength trace: D6), B-080 and B-157 (cheatsheet / paper
   netlist), B-155 (error locations). Do not change behaviour in this item.
2. **MLPROBE per-event print.** `src/devices/probe.cpp` prints "received unsupported signal" on stdout for every
   event at an unlisted wavelength (a print in a hot path). Replace it with one warning per probe at the end of
   the run on stderr, in name order (like the D5 latch warnings: which probe, which wavelengths it ignored).
   Predicted: the cases whose stdout has those lines (list them) lose them and gain the stderr line; nothing else.
3. **`SC_HAS_PROCESS`** (deprecated in IEEE 1666-2023, about 36 uses): remove or replace as SystemC 3.0.2
   recommends. Bit-identical outputs, counters identical; no new warnings.
4. **B-182** (error line numbers inside `.subckt` bodies): fix as its file suggests. Predicted: only the messages of
   affected error cases change (list them).
5. **Small items:** `src/test/devices/detector.cpp` `memcpy` from an empty vector on the failure path (UB);
   `-Wdangling-else` in `src/test/optical_output_port.cpp`; the stale "Not gating yet" comment in
   `.github/workflows/ci.yml`; obsolete `doc/Doxyfile` tags reported by Doxygen 1.18 (remove only tags that are
   obsolete; the docs must still build with the Ubuntu apt Doxygen in CI); `tests/golden/README.md` case counts.
6. **Look, do not change:** VLSource prints every emission on stdout. Report whether this is user-facing output
   worth keeping or a per-event debug print, with the golden impact a change would have, under "Found, not done".
