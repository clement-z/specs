# PK — fixes before v0.4.0 (report)

Branch `ws/kfixes`, from `sprint/2026-10c` at 22714f2. Decision: "Before v0.4.0 (2026-10-23)" in
`backlog/DECISIONS-2026-10-21.md`. Progress and predictions: `PK-progress.md`. Nothing pushed.

## Items
| # | Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|---|
| 1 | B-189 negative CWSRC power | done | 634c886 | new `err/b189-cwsrc-negative-power` | 1 new case |
| 2 | B-193 run-table test in CI and pixi | done | a8b4721 | run-table OK in `pixi run test gcc` and `asan-clang` (was SKIP) | none |
| 3 | B-191 long netlist lines | done, REJECT removed (no fallback needed) | 8fc95cb | new `regress/b191-long-line-vlsrc`; 35-netlist before/after differential check; independent review (below) | 1 new case |
| 4 | B-192 value files resolved as `.include` | done | dd822d7 | new `regress/b192-value-files-next-to-netlist`; gtest `ParserStateDeathTest.ValueFilesResolvedAsIncludes` | 1 new case, `gtest/unit_tests` re-recorded (new test) |
| 5 | Remove the legacy CROW testbench | done | 3337f4c | oracle suite without `--legacy-tb`; 3 replacement cases | 5 deleted, 4 re-recorded (one line each), 3 new |

Every golden change was predicted in `PK-progress.md` before recording and matched.

### 1. B-189
`CWSourceElement::create` checks the power (positional and `POWER=`) before `setPower`:
`Error (ROOT/CWSRC1): power must be >= 0 and finite (got -1 W)`, exit 1 (was SIGABRT in `std::polar` with
`_GLIBCXX_ASSERTIONS`, a NaN field otherwise). **B-063 (negative VLSRC values) not included**: its fix has an open
design question. `circuits/subckt_tests.cir` feeds `-2` W, and `cir/subckt_tests`/`opt/vci` bake in the NaN. Editing the
example or clamping is the maintainer's choice (B-135). Non-finite powers cannot come from the netlist today: `1e400`
is read as 1.8e308 (see "Found, not done") and `{1/0}` is a syntax error. The check still rejects them.

### 2. B-193
numpy and pandas are now in pixi's default `[dependencies]`. `pixi.lock` was updated with pixi 0.82.1 (the CI
version): additions only (numpy, pandas, BLAS/LAPACK), no other package changed. `pixi install --locked` OK. The GitHub
`build-test` and `sanitizers` jobs and the GitLab `test` job pip-install them next to Meson. The CI pixi job gets
them from the lock. **Why this way:** the Python that `meson test` already finds (`find_program('python3')`) is the one
that has them, so no Meson logic has to locate another environment. The `py` environment is installed neither by
`pixi install` nor by the CI pixi job, so pointing at it would not be robust. The skip (exit 77) is kept for
plain builds without them. The activity-map test now imports numpy, but its render part still skips (no
matplotlib in the default environment): unchanged.

### 3. B-191
Flex used its REJECT mode for the three REJECTs **and** for two variable trailing contexts (`/{EOL}` with
`EOL = \r?\n`, `/{WS}*=`). Flex also detects the word REJECT in action comments. All removed:
- line rule `^.*/\n` (the recorded line keeps a `\r`, as before: flex took the longest head): records the line,
  `yyless(0)`, `yy_set_bol(1)`, then a twin start condition (`INITIAL_L`, `INCLUDE_L`, `SUBCKT_DECLARATION_L`,
  `SUBCKT_DEFINITION_L`, `IN_DQUOTES_L`; inclusive/exclusive as the original) that has every rule but the line
  rule. `YY_USER_ACTION` returns to the original state at the next match, before its action. This gives the
  match that REJECT gave;
- `.subckt`/`.ends` inside a body append `yytext` (the fallback rule appended the same characters one by one);
- `.TITLE`: `/\n` and `/\r\n` rules; `NAME{WS}*=`: matched whole, name kept with `yyless(strcspn(yytext,
  " \t=\xC2\xA0"))` (`{WS}` contains the bytes of U+00A0).

The generated scanner has no `yy_state_buf`/`yy_full_match` and no "can't enlarge" path. Flex grows its buffer, so
there is no line length limit any more, and the "enlarge the buffer + clear error" fallback was not needed. Golden:
all existing cases bit-identical. Differential check (old vs new binary, stdout+stderr+exit) on 35 edge netlists
(CRLF, `.title` forms, continuation lines, NBSP, nested subckts, last line without EOL, includes, quotes, error
lines): identical except a 30 KB comment line (failed before). Long lines also parse in a subckt body and in an
included file (checked by hand). The new case's stdout is 292 KB (the element listing of the 2000 values); kept as
proof that every value is read.

Independent review (Opus subagent, high effort, read-only). The static review of the twin states against flex
2.6.4 found them complete and consistent: inclusive/exclusive as the originals, state stack, BOL, `yylineno`,
EOF, no infinite loop. The differential test ran about 92,000 inputs in file and string modes, with token-dump
drivers linking the old and new generated scanners and checking tokens, values, `yylineno`, the recorded line and
the exit code. It also ran 1,300 random netlists on the full binaries. Everything was identical except lines over
16 KB. It found two limits, recorded under "Found, not done": scan time quadratic in the line length, and bison's
stack on long lists.

### 4. B-192
`VLSourceElement`/`EVLSourceElement` keep `include_dir` (set by the parser as for SPAR; in a `.subckt` body, the
file that defines the subcircuit). `create()` and `data_files()` open/report `resolve_data_file(include_dir,
file)`, which is `ParserState::resolve_include()`, as SPAR. So the manifest keeps "the path used to open". Existing
cases: the 8 value-file references of the golden netlists are working-directory paths with no file of that
name next to their netlist (checked by script); the param-named ones (`cir/paul_3ring_puf`, `err/b064-*`)
name files that exist nowhere, so the error text is unchanged. Baseline manifests unchanged. CHANGELOG (Changed): a
file of the same name next to the netlist now wins. Docs: `doc/netlist.md` (VLSRC, EVLSRC), `doc/reproducibility.md`.

### 5. CROW testbench
Removed: `src/tb/crow_tb.{cpp,h}`, its `tb_map` entry, `--nrings_crow` (only crow_tb used it), the oracle's
`crow-tb` check and `--legacy-tb`, `backlog/repro/a2-vci-crow.sh`. Golden: `tb/ac_crow`,
`regress/b036-vci-crow`, `regress/b131-nrings-crow`, `err/b131-nrings-crow-zero` and `regress/f15-stats-testbench`
were deleted (every case running `-t ac_crow`). The help line and the testbench-list line went from `cli/help`,
`cli/no-args`, `err/f15-no-coalescing-removed` and `cli/list-testbenches`. Replacements:
- `regress/crow-tb-preset-n3`: the `tb/ac_crow` circuit, written by `crow_gen.py 3 --preset crow_tb`;
- `regress/crow-tb-preset-n8-vci`: `--vci` on 8 rings (what `b036-vci-crow` and `b131-nrings-crow` covered);
- `regress/f15-stats-testbench-wg`: `--stats=ports` with `-t wg` (stats file next to a testbench VCD).

The `"net": null` case (ports bound to signals destroyed with the testbench function) has no other testbench. The
existing gtest `RunStats.PortOfADestroyedSignalHasNoNet` covers it. Docs: `tests/bench/README.md`, `crow_gen.py`,
`tests/golden/README.md` (counts, also those of items 1-4), `meson.build` comment, `discover/01, 02, 04, 05`
(annotated, not rewritten), CHANGELOG (Removed), notes in B-025, B-036, B-071, B-131, B-140.

## Decisions taken
- B-063 left out (open design question, above).
- B-193: numpy/pandas in the default pixi environment and in CI rather than the `py` environment (above).
- B-191: no buffer enlargement or error message: REJECT removal was possible with bit-identical outputs.
- B-192: same warning text as `.include`/SPAR when both files exist and differ (`Warning: .include P: using
  ...`), i.e. exactly the shared resolver. Clearer wording is under "Found, not done".
- B-192: `gtest/unit_tests` re-recorded with the new case, as earlier phases did for new gtests.
- Item 5: the `CROW` class is kept (not part of the testbench; "Found, not done"). Historical records (scout briefs,
  orchestration prompts, sprint reports, `discover/meson/DECISIONS.md`, fixed backlog items) are left as they
  are. Replacement cases are generated netlists committed with a "regenerate, do not edit" header. N = 8 for the
  `--vci` case (many rings); its VCD (519 KB) is stored. The baseline grows from 3.7 MB to 4.6 MB in this phase.
- Item 5 commit subject `refactor: ...` (no backlog id).

## Found, not done
- The `CROW` class (`src/devices/crow.{h,cpp}`) has no user left; `src/tb/freqsweep_tb.h` includes `crow.h`
  without using it. The `--vci` print path for ports bound to non-`spx` channels (`src/utils/sysc_utils.h`) is
  reached by no golden case any more. Remove the class, or expose it as a netlist element?
- `ParserState::resolve_include()`'s warning says `.include P: ...` for SPAR files and value files too.
- Floating-point literal overflow is silently clamped: `power=1e400` is read as 1.79769e+308 (B-180 made integer
  literals errors, not doubles).
- B-063/B-135 (negative VLSRC values, `circuits/subckt_tests.cir`) still need the maintainer's decision.
- **Long inline lists hit bison's stack** (reproduced): a VLSRC with 5,000 inline `[t, P, wl]` values (about 140 KB)
  fails with `Parsing error (...:1): memory exhausted`, exit 1; 4,000 parse. The list rule is recursive and bison's
  default stack is 10,000 deep. Not the scanner (B-191); a value file has no such limit. Noted in B-191.
- Scanning a single line is now quadratic in its length: flex rescans the partial line token at each buffer
  refill. 1 MB takes 0.4 s, 8 MB 24 s; these lines failed before. Negligible at realistic sizes (the 55 KB case
  costs nothing).

## Final results
- `pixi run test gcc`: 17/17 OK (run-table no longer skipped). `pixi run test asan-clang`: 17/17 OK, no
  sanitizer message (also after items 1, 3, 4).
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 276 passed (275 cases + gtest), 0 failed/new/missing.
- clang and asan-gcc against the baseline (strict): the same 7 known last-bit cases fail before and after (AGENTS.md).
  Before/after `compare` (strict): 4 re-recorded (item 5) and 6 new cases on both, nothing else.
- `stats-compare` (gcc) before/after: 269 same, 0 differ, 6 new.
- Heartbeat `690a2092` deleted after this report.
