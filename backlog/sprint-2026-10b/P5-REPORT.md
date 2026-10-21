# P5 report — F-28 `.op`/`.dc` convergence + F-12 netlist pack (branch `ws/netlist`)

Sprint 2026-10b, phase P5, on top of `sprint/2026-10b` @ 0dee6b2 (P1–P4 merged). Prompt:
`discover/orchestration/2026-10b/P5-netlist.md`. Everything decided for F-28 and F-12 is **done**. Nothing was
pushed and nothing is held back for sign-off. The only result change is the decided one: every `.dc` sweep whose
(stop − start)/step is an integer gains its stop point (10 golden cases, each checked to be the old output plus
that point).

How the work was split: the orchestrator did F-28, the legacy-netlist move, all golden recording and the
bookkeeping. Two Opus implementers ran in parallel in `/tmp/P5-wt/lexgram` (`P5-wip/lexgram`: SI suffixes,
`.ends NAME`, `.dc ATTR(elem)`) and `/tmp/P5-wt/semantics` (`P5-wip/semantics`: `.dc` stop value, `--param`,
`.include`), committing code, unit tests, netlists and `cases.json` entries only. A Sonnet writer did the syntax
section of `doc/netlist.md` in `/tmp/P5-wt/docs`. The orchestrator reviewed every diff, cherry-picked the commits
one by one (merging `cases.json`/`meson.build`), re-checked and recorded the golden cases per commit, and amended
the golden lines of the commit messages.

## Items

| # | item | outcome | commit | tests | golden impact |
|---|---|---|---|---|---|
| 1 | F-28 iteration cap, residual report, non-convergence error | done: `SPECSConfig::runFixedPoint()` steps the delta cycles of the current time step with `sc_start(SC_ZERO_TIME)` (the same delta sequence as `sc_start()`), at most `maxiter` of them per solve (the `.op`, each `.dc` point, the OP before `.tran`); default 1000000, `.options maxiter=N` (0..2^53), `--maxiter N` (applied directly like `--seed`, wins over the netlist), 0 = no limit. On non-convergence: `Error: <solve> did not converge within N iterations (delta cycles, .options maxiter)`, the residual (ports that emitted in the last tenth of the iterations: net, writer, wavelength, last \|dE\| and its threshold max(abstol, reltol·\|E_last\|), largest order of magnitude first, then name order, 10 listed) and a hint; exit 1. Ports record their last emission (a few stores per emitted event). Follow-up fix: residual order by order of magnitude (clang/gcc last-bit differences) | 1be5a5c, 0a5fe9f | gtest `FixedPoint.*` (lossless loop stops at the cap and reports both nets; a converging circuit stopped by a cap of 2 resumes to the same OP; `maxiter` 0; default); 9 golden cases | new `err/f28-lossless-ring-{op,dc,tran}`, `err/f28-maxiter-small`, `err/f28-options-maxiter-{negative,fraction}`, `err/f28-maxiter-cli-invalid`, `regress/f28-maxiter-cli-priority`, `regress/f28-maxiter-zero` (identical to `regress/b124-allpass-ring-default-reltol` except the manifest's command line); re-recorded `cli/help`, `cli/no-args` (help lines), gtest. No converging case changed |
| 2 | F-12 SI suffixes (B-160) | done: `f p n u m k meg g t` directly after a number, caseless (`M` = milli, `MEG` = mega); a suffixed number is a DOUBLE; the suffix shifts the decimal exponent before conversion, so `1.55u` is bit-identical to `1.55e-6` (`1.5e3k` = `1.5e6`); everywhere numbers are lexed (kwargs, positional args, `{...}`, `.param`, `.options`, analyses, `--param`) | 6c837e5 | `regress/f12-si-suffixes` (JSON; `rel_*` params check the exact rounding: all 0), `regress/f12-si-op`, `err/f12-si-unit-letters` | 3 new cases |
| 3 | F-12 `.ends NAME` (B-169) | done: optional name, compared case-insensitively; mismatch is a parse error with file:line (`.ends rong does not match the name of the subcircuit (.subckt RING)`), exit 1 | e80bba4 | `regress/f12-ends-name` (nested, mixed case, bare `.ends`), `err/f12-ends-name-mismatch` | 2 new cases |
| 4 | F-12 `.dc WL(src)` alias | done: `ATTR(elem)` is the same `(element, attribute)` pair as `/elem/ATTR` (WL, P, F and their long forms) | b00b8ae | `regress/f12-dc-wl-alias` and `regress/f12-dc-wl-slash` (identical stdout and VCD), `regress/f12-dc-p-alias`, `err/f12-dc-alias-unknown-attr` | 4 new cases |
| 5 | F-12 `.dc` includes the stop value | done: `sweep_count()`/`sweep_values()` (`utils/general_utils.h`): n = floor((stop − start)/step + 1e-9) + 1 points, accumulated exactly as before, so every existing point keeps its bits and the stop is appended when the ratio is an integer; the parse-time count is the swept count; `range()` untouched | a67deb3 | gtest `GeneralUtils.*` (add_drop: 31 points, first 30 bit-identical to `range()`; integer and non-integer ratios; 1–2 points; invalid inputs) | **one-time re-record, as decided**: `cir/add_drop`, `cir-json/add_drop`, `opt/tol-resolution-overrides`, `opt/mode-fd`, `opt/multi-file`, `regress/b004-mode-td-dc`, `err/b017-json-unwritable` (`(30points)` → `(31points)`, and when simulated `(31 points)`, one more `emitted` line, one more VCD block `#30` at 1565 nm) and the 3 new `.dc` cases of item 4 (4 → 5, 3 → 4 points). Checked case by case with `/tmp/P5-tools/verify_stop.py`: the old output is an exact prefix, stderr/manifests/JSON byte-identical |
| 6 | F-12 `--param name=value` | done: repeatable; value parsed by the netlist grammar into a scratch tree (free expressions, suffixes, strings); overrides the root-level `.param`/`.assign` of that name wherever it is; subcircuit parameters untouched; a name the netlist never assigns is still defined and warns `Warning: --param <name>: the netlist has no .param <name>`; malformed: `Invalid --param value: '<arg>' (expected name=value)`, exit 1 | a64b445 | `regress/f12-param-override`, `regress/f12-param-undefined`, `err/f12-param-malformed` | 3 new cases; `cli/help`, `cli/no-args` |
| 7 | F-12 `.include` relative to the including file (B-143) | done: relative path → dir(including file)/P, then P (cwd); the including file of a subckt-body include is the file that defines the `.subckt`; nested includes keep a directory stack; a warning when both candidates exist and are different files with different contents; `Including file:` and the manifest show the path opened; existence checks by `stat` (no fd) | 062399c | gtest `ParserState.*`; `regress/f12-include-relative` (main → mid → sub/leaf, sub/probes after the pop, subckt body include next to its defining file), `regress/f12-include-both-differ` | 2 new cases; every existing include resolves through the cwd fallback, unchanged (b150, b156 under 64 fds, f01-manifest, subckt cases) |
| 8 | F-12 legacy ring netlists | done: `git mv` to `circuits/legacy/` + `circuits/legacy/README.md` (pre-release syntax, kept for reference, not maintained); `.gitignore` re-includes `circuits/legacy/*.cir`; case ids kept | 829894f | — | `cir/double_ring`, `cir/triple_ring`, `cir/paul_simple_ring` re-recorded: only the path (args, parse-error message) |
| 9 | docs, backlog, report | done: `doc/numerics.md` ("Convergence of `.op` and `.dc`", `maxiter` row), `doc/netlist.md` (new "Netlist syntax" section), README, CLAUDE.md (include rule), CHANGELOG `[unreleased]`, golden README (175 cases); backlog B-160, B-169, B-143, B-133 fixed, B-009 and B-080 partly, with their `backlog.md` rows | 1be5a5c, 0a5fe9f, 50f3266, bb1b7e5 | — | — |

## Decisions taken (not covered by the decision record; none changes results beyond the decisions)
1. **One iteration = one delta cycle** of the time step being solved (the decision says "delta-cycle fixed point").
   Counted per solve: the `.op`, each `.dc` point, the OP before `.tran`. A transient run with `-m fd` is not
   capped (not an `.op`/`.dc` fixed point).
2. **Default `maxiter` = 1000000.** Measured: the golden suite needs at most 869 delta cycles per solve (the B-124
   all-pass ring), a chain of 1000 such rings ~1e5, a 10-ring CROW 3.5e4; 1e6 iterations take ~0.25 s on a small
   circuit, ~30 s on a 100-ring CROW (which, like a 20-ring one, does not converge even in 1e8). Names: `.options
   maxiter`, `--maxiter`; 0 = no limit.
3. **`--maxiter` is applied directly** (like `--seed`: full 64-bit range, also when the netlist sets `.options
   maxiter`, which it overrides), not through the `.options` footer, because the lexer clamps integer literals
   beyond 2^31 (P4 finding).
4. **Nothing is printed for a converging solve** (no iteration count, no new `printConfig()` line, no VCD config
   trace), so converging cases are byte-identical. An idle `.op` keeps one `sc_start()` call so that SystemC's
   W571 "no activity" warning of the old code stays (9 golden cases print it).
5. **Residual**: ports that emitted in the last tenth of the iterations (at least the last 100 delta cycles),
   sorted by order of magnitude of last change / threshold, then name (exact ratios differ between compilers in
   the last bits); 10 listed, then "... and N more"; on stderr.
6. **SI suffixes**: no trailing unit letters (`1.55um`, `10mW` stay syntax errors; `mil` not supported).
7. **`.ends NAME` mismatch is an error** (not a warning), compared case-insensitively.
8. **`.dc ATTR(elem)`** accepts every attribute of `/elem/ATTR` (`P(...)`, `F(...)`), not only `WL(...)`.
9. **`.dc` stop**: values keep the accumulation (B-009's start + i·step would move every point: not decided);
   inclusion slack 1e-9 of a step; consequences: a single interval gives 2 points and is now accepted; stop ≤
   start, step ≤ 0, NaN/inf are rejected at parse time with `Invalid range parameters` (before: `Sweep order must
   contain at least 2 point.` for stop ≤ start; step = 0 printed `(infpoints)` first). No golden case covered
   those messages.
10. **`--param`**: last one wins; names are case-sensitive (as `{name}` lookups); values cannot use netlist
    parameters (parsed before the netlist), only literals, expressions and `pi`/`c`; overriding `pi`/`c` is
    allowed (with the warning); ignored with `-t` (like `--reltol`).
11. **`.include` warning** only when both candidates exist, are not the same file and differ in content (identical
    copies: no warning); a directory is not a candidate; paths are not normalised. Quoted names not handled.
12. **Legacy netlists**: the golden case ids `cir/double_ring`, ... are kept (only the path changes).

## Found, not done
- Parse errors inside a subcircuit body print an empty file name and a garbage line number (`Parsing error
  (:1634561392)`): the per-instance re-parse (`ParseTree(name, subcircuit, kwargs)`, `parse_tree.cpp`, `yy_scan_string`)
  never sets the line number. Also hits a nested `.ends` mismatch, which therefore has no golden case. Related to
  B-155 (likely fix: `yyset_lineno(1, scanner)` plus the defining file's name/offset).
- A nested `.ends` name (and any error in a body) is only checked when the outer subcircuit is instantiated.
- `.endsring` (no space) is read as `.ends ring`, like `.tran1` = `.tran 1` (lexer keywords have no word boundary).
- `.include "file.cir"` (quoted, common in SPICE) is still not supported (B-143's suggested fix mentions it).
- Errors inside an included file name the including file (B-155). Line numbers of `-f` files are correct (checked:
  first and second `-f` file).
- The parser is not linked into `unit_tests`, so lexer/grammar behaviour is only tested through golden cases.
- `sweep_count()` has no upper bound: a tiny step gives a huge allocation / undefined cast (the old `range()` had
  the same with an `int`).
- A 1000-ring chain netlist takes ~80 s, with ~1e5 delta cycles only: the time is spent outside the fixed point
  (same with the old engine, 86 s). Worth a profile (F-15).
- `-m fd` with `.tran`: the whole transient is a frequency-domain fixed point and is not capped by `maxiter`.
- `.options maxiter=3000000000` is silently 2147483647 (integer-literal clamp, held back in P4 on
  `P4-ready/lexer-int-overflow`); harmless here, `--maxiter` takes the full range.
- The `Recorded sweep order ... (31points)` message has no space before "points" (kept to limit the diff).
- `doc/netlist.md` still has no argument sections for WG, MERGER, SPLITTER, PSHIFT, MZI and PWR (the new prefix
  table points to the cheatsheet PDF for them); the PDF itself is unchanged (B-080).
- Grammar limits found while documenting (documented, not changed): a positional argument takes no free
  expression and no negative number (`cwsrc1 in 1.55u -3`, `.tran 2*5n` are syntax errors; `{-3}`, `{2*5n}` work);
  a keyword takes no braced expression (`length={ {x}*2 }` fails, `length={x}*2` works); `.dc` start/stop/step take
  only a number or `{name}`; elements must start in column 1; an unexpected character exits 3, not 1.
- `.op` writes only the run manifest in `traces/`, no VCD (also noted in B-080).

## Final results
On HEAD (code as of 0a5fe9f; later commits are docs only):

| build | `meson test` | golden `check --atol 0` (strict order, NEW/MISSING fail) |
| --- | --- | --- |
| gcc 11.5, `/tmp/P5-gcc` (incremental, release) | 12/12 | 176 passed (175 `specs` cases + gtest), 0 notes |
| gcc 11.5, `/tmp/P5-gcc-fresh` (fresh setup) | 12/12 | 176 passed, 0 notes |
| clang 19.1.7, `/tmp/P5-clang` (fresh setup) | 12/12 | 176 passed, 0 notes |
| clang 19.1.7 ASan+UBSan, debug, `/tmp/P5-asan` (`ASAN_OPTIONS=detect_leaks=0`) | — | 176 passed, 0 notes (no sanitizer output on any stderr) |

Golden baseline vs the start of the phase (0dee6b2): 23 new cases (9 `f28-*`, 14 `f12-*`); re-recorded: the 7 `.dc`
cases (stop value), `cli/help` and `cli/no-args` (`--maxiter`, `--param`), the 3 legacy cases (path), gtest (+13
tests, 68 → 81, all PASS: `FixedPoint.*` 3, `GeneralUtils*` 5, `ParserState*` 5).
Every partial re-record is in `baseline/manifest.json` `rerecords` with its note.

Implementer branches kept: `P5-wip/lexgram`, `P5-wip/semantics`, `P5-wip/docs` (all cherry-picked); their
worktrees under `/tmp/P5-wt` are removed. Heartbeat `50c26c63` is deleted after this commit.
