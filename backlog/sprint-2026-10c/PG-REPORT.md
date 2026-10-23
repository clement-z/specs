# PG report: cleanup after v0.3.0

Branch `ws/cleanup3` (from a761d28, the phase G prompt on top of v0.3.0). Prompt:
`discover/orchestration/2026-10c/PG-cleanup.md`. Unattended run of 2026-10-23. No `git push`, no merge.

All five items are done, and item 6 was looked at without changes. Every output change matches its prediction
(written in `PG-progress.md` before the change): no existing golden case changed, and five new cases were added.
The `--stats` counters are identical on every existing case for gcc, clang, asan-gcc and asan-clang.

## Items
| # | Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|---|
| 1 | Backlog statuses | done: 3 fixed (B-057, B-108, B-130), 13 partly (B-015, B-022, B-046, B-069, B-071, B-078, B-084, B-104, B-116, B-139, B-155, B-157), every other open item checked and still open | 70fc144 | — | none (doc only) |
| 2 | MLPROBE per-event print (B-062) | done: one warning per probe on stderr at the end of the run, in name order, full precision | 5b946f2 | gtest `MLambdaProbe.ReportsIgnoredWavelengthsOnce` (new), 2 MLambdaProbe tests updated; golden `regress/b062-mlprobe-unlisted-wavelength` (new) | predicted and observed: no existing case changed (none printed the line) |
| 3 | `SC_HAS_PROCESS` (B-030) | done: the 38 uses removed | 9f31c11 | clean-build warnings: clang 455 → 4, gcc 9 → 9 (none new) | none: bit-identical before/after on gcc, clang, asan-gcc and asan-clang; counters identical |
| 4 | B-182 line numbers in `.subckt` bodies | done: defining file and line, nested bodies included | 6561510 | golden `err/b182-subckt-body-syntax-error`, `-int-literal`, `err/b182-nested-subckt-ends-mismatch`, `-body-error` (new) | predicted and observed: no existing case changed (none had an error in a body) |
| 5 | Small items | done: `memcpy` guard, `-Wdangling-else`, CI comment, Doxyfile, golden README counts | 69c774f | docs built with Doxygen 1.9.8 and 1.18 | none |
| 6 | VLSource prints (look only) | reported below under "Found, not done" | — | — | — |

Bookkeeping commits: 6e5e83c, 7ea3971, 3c9cf2c (progress and predictions), 5c1bbcb, 3ec716e, 83b1e4e, add6336 (statuses and progress after
each item). The CHANGELOG `[unreleased]` section has B-062 under "Changed" and B-182 under "Fixed". `doc/netlist.md`
documents the MLPROBE warning.

### 1. Backlog statuses
Three Sonnet subagents (effort medium) checked 72 items against the code, the 2026-10b/c reports and git history.
They edited only the status line of each issue file. I reviewed every diff, checked that every cited SHA is an
ancestor of HEAD, and wrote the `backlog.md` cells. The known candidates came out as follows:
- B-057 and B-108: fixed.
- B-015: partly. D2 removed the heap allocation per notify; the linear scan of the pending entries remains.
- B-078: partly. Since D6, `wl=0` no longer drops a PROBE's wavelength. The `has_wl` merge and the "wavelength as
  first signal" failure remain in pyspecs, and failure 3 is still reachable through `.save P(net)` alone.
- B-080: unchanged ("partly"). The PDF cheatsheet is unchanged and MZI is still undocumented.
- B-157: partly. The `dc1` prefix and `{expr}` bounds remain.
- B-155: partly. Subcircuit bodies were fixed by item 4; included files, kwargs and elaboration errors remain.

Items fixed by this phase were updated in their own step: B-030, B-062 (partly: matching stays exact), B-182, and
B-066 (partly: only the MLPROBE print is gone).

### 2. MLPROBE
`MLambdaProbe` now records each ignored wavelength in `m_ignored_wavelengths_m`, a set insert, in place of the per-event
`cout`. `SPECSConfig::runAnalysis()` reports them after any analysis (`.op`, `.dc` or `.tran`) for every MLPROBE in
name order. Each warning is one line:
`Warning: ROOT/MLPROBE1 ignored the wavelengths of its net ROOT/a that it does not list (1.5514999999999999e-06 m;
listed: 1.5515e-06 m)`. Wavelengths are printed as the shortest decimal that reads back as the same double, so a
near miss (the B-062 repro) is visible.

### 3. `SC_HAS_PROCESS`
SystemC 3.0.2 defines `SC_HAS_PROCESS` as a no-op `static_assert` on a `[[deprecated]]` variable (`sc_module.h:413-418`).
`SC_THREAD`, `SC_METHOD` and `SC_CTOR` use `decltype(*this)` instead, so the uses can simply be deleted, which is
what SystemC recommends. Only the macro lines changed, plus the blank line after the 31 uses that opened a body.

### 4. B-182
In a reentrant flex scanner the line number belongs to the buffer, and a `yy_scan_string` buffer never initialises
it. The fix:
- The lexer records the body's first line (`ParserState::subckt_body_line`).
- The parser stores it in `ParseSubcircuit`, together with `yyget_filename()`.
- The per-instance scanner starts from that line and file (`yyset_lineno`, `current_filename`). Nested bodies
  inherit the offset.

A syntax error whose offending token is the end of a line reports the next line, the same as for top-level lines.
The new case documents this in its header.

### 5. Doxyfile
Doxygen 1.18 reports 12 tags as obsolete. Doxygen 1.9.8 (Ubuntu 24.04 apt, used by CI) reports 11 of them; the
12th, `DOT_MULTI_TARGETS = NO`, equals its default there. All 12 were removed with their comment blocks. Both
versions had been translating three of them into current tags, and `doxygen -x` showed the translations, so I
wrote those current tags explicitly:
- `TIMESTAMP = YES`
- `DOT_COMMON_ATTR = "fontname=Helvetica,fontsize=11"`
- `DOT_EDGE_ATTR = "labelfontname=Helvetica,labelfontsize=11"`

Results:
- `doxygen -x`: the effective configuration is identical to before with 1.9.8 and with 1.18.
- Full builds: rc 0 with both versions, no "obsolete" warning, the other warnings unchanged (1.9.8: 74; 1.18: 71).
- The HTML differs only in dot-graph layout, which also varies between two runs of the same configuration.

## Decisions taken (conservative choices)
- **Item 2, reporting point:** one call after the analysis switch in `runAnalysis()`, so it covers `.op`, `.dc` and
  `.tran`. The `-t` testbenches run no netlist analysis, and their probes are destroyed when the testbench function
  returns, so the destructor reports whatever was not reported yet. This keeps their information, which used to go
  to stdout. A probe never warns twice: reporting clears the set.
- **Item 2, scope:** matching stays exact. A tolerant match is the needs-design part of B-062 and is left open.
- **Item 4:** the file name is the one top-level errors use (`yyget_filename`). Inside an `.include`d file this is
  the including file, as for top-level lines (B-155, open).
- **Item 5:** `err/b182-nested-subckt-body-error` first used an unexpected character, which exits 3 (B-172), the
  only `err` case not exiting 1. I switched it to a syntax error (exit 1) so that the README's "exit 1" stays true.
  Only that case was re-recorded.
- **Item 5, Doxyfile:** the three translated tags were kept as their current equivalents rather than dropped, so
  the docs stay identical (timestamps, font size 11). Doxygen 1.9.1 (Ubuntu 22.04, and this host's
  `/usr/bin/doxygen`) still builds the docs, but warns "ignoring unsupported tag" for the three and uses its defaults.
- **Baseline bookkeeping:** three `record --only` runs, each with a note: the b062 case plus the gtest list, the four
  b182 cases, and the nested b182 case again. No existing case was re-recorded.

## Found, not done
- **Item 6, VLSource / EVLSource prints.** `VLSource::runner()` writes to stdout:
  - "`<name> was enabled`" once per source;
  - "`@<t>, <name> emitted: <value>`" for every sample of its value list, plus the zero emitted at enable;
  - "`invalid time for signal emission !`" for a sample in the past.

  The file-reading constructor also prints every sample it parses (`value_list_source.h:123`) and each skipped
  line. EVLSource is a copy (B-074).

  **Verdict:** this is a per-sample debug log, not user-facing output worth keeping as is:
  - It is not printed per engine event; its size is bounded by the input. Still, a value file of 1e5 rows prints
    2e5 lines.
  - The values are already in the traces whenever the net is probed.
  - Nothing reads it: pyspecs, the docs and the bench scripts do not.
  - Only the "invalid time" line is a real diagnostic, and it belongs on stderr as a warning.

  **Suggestion (B-066, needs-design):** put the per-sample lines and "was enabled" behind `--vci` or a verbosity
  option, and move "invalid time" to stderr.

  **Golden impact of that change (stdout only):**
  - 35 cases lose 134 VLSRC emission lines and 3 cases lose 17 EVLSRC lines;
  - 41 cases lose 77 "was enabled" lines;
  - `regress/b052-vlsrc-positional` loses its parse-time sample listing;
  - no case prints "invalid time".

  CWSource's per-point "emitted" line (85 cases, 765 lines) is the other half of B-066. Values and traces would be
  unchanged.
- **Offending token at the end of a subcircuit body:** when a syntax error's offending token is the end of the body
  string, the token prints as a raw newline instead of `\n`. This predates the fix and is cosmetic; the new case
  avoids it.
- **Remaining build warnings (pre-existing):** gcc `-Wstrict-aliasing` in `specs.cpp:721-724` (B-029); clang
  `unused variable 'next_emit'` (×2), plus `yynerrs` and `-Wmisleading-indentation` in the generated parser and
  scanner.
- **Historical SC_HAS_PROCESS counts:** `discover/01`, `02` and `04` still quote these counts as findings of their
  time. Left unchanged.
- **Subagent hesitations, for a human to re-scope:**
  - B-042: the golden-ID premise is gone, because ids are off by default, but the dead init is still there.
  - B-007: the E701 text is gone, but the cwd-relative default path remains.
  - B-070 and B-116 point 1 (SystemC `-isystem` in the wrap build) were not verified in depth.
- **Process note:** `/tmp/pg/before/<toolchain>` copies of `specs` need `subprojects/systemc-3.0.2/libsystemc.so*`
  next to them, because the rpath is `$ORIGIN/...`. The first "before" recordings ran with exit 127 and were redone
  before any comparison was used.

## Final results (tip after item 5, sources unchanged since 69c774f)
- `pixi run test gcc | clang | asan-gcc | asan-clang`: `meson test` Ok 12, Fail 0 on each, with no sanitizer message.
  The sanitizer runs use `tests/sanitizers/lsan.supp` unchanged.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 240 passed, 0 failed, 0 new, 0 missing (239 `specs`
  cases + gtest).
- `pixi run golden clang | asan-gcc | asan-clang` (default tolerances): 240 passed each.
- Before/after: `run_golden.py compare --rtol 0 --atol 0 --print-slack 0` and `stats-compare` against recordings of
  the pre-phase binaries (6e5e83c). On all four toolchains the 234 existing `specs` cases are bit-identical and
  their counters identical. The only differences are the five new cases and `regress/b062-…`, which the pre-phase
  binary ran with the old per-event lines.
