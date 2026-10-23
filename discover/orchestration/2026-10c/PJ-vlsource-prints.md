# Phase J — value-list source prints (branch `ws/vlprints`)

Follow `COMMON.md` and the gate of `PD-common-f15.md`. Phase id `PJ`. Decision "Follow-ups (2026-10-23)" in
`backlog/DECISIONS-2026-10-21.md`; analysis in `backlog/sprint-2026-10c/PG-REPORT.md` ("Found, not done", item 6)
and backlog B-066.

1. `VLSource` and `EVLSource` (and the value-file reading constructor, `value_list_source.h`): the per-sample
   lines ("`@<t>, <name> emitted: <value>`", the parsed-sample listing, skipped-line notes) and "`<name> was enabled`"
   are printed only with `--vci` (verbose component initialisation), on stdout as today. "`invalid time for signal
   emission !`" becomes a warning on stderr, always printed, once per source with a count (not per sample).
   CWSource's per-point line is **not** in scope.
2. Predicted golden impact (write the exact list in `PJ-progress.md` before the change; PG-REPORT counted: 35 cases
   lose 134 VLSRC emission lines, 3 lose 17 EVLSRC lines, 41 lose 77 "was enabled" lines, `regress/b052-vlsrc-positional`
   loses its parse-time listing; no case prints "invalid time"): stdout only, lines removed, nothing else; values,
   traces and counters identical. Prove it (old stdout minus the removed lines equals the new one). Add a golden case
   with `--vci` showing the lines are still available, and one for the stderr warning.
3. Docs (`doc/netlist.md` VLSRC/EVLSRC, `--vci` in the CLI table), CHANGELOG `[unreleased]` (Changed), B-066 status.
