# P2 report — netlist and engine correctness (branch `ws/engine`)

Sprint 2026-10b, phase P2, on top of `sprint/2026-10b` @ d214196 (P1 merged). Prompt:
`discover/orchestration/2026-10b/P2-engine.md`. Eight of the nine items are **done**; item 3 (B-146) is **stopped**
(an unpredicted golden change) and parked, ready to land, on branch `P2-ready/b146`. Nothing was pushed.

Work split: Opus implementers for items 2–3 (bidir), 4–6 (positional/PCM) and 7–8 (CLI), Sonnet for item 9 (docs),
each in its own `/tmp/P2-wt/<name>` worktree (branches `P2-wip/*`). They committed code, tests and netlists only; the
orchestrator reviewed every diff, cherry-picked it onto `ws/engine`, added the golden cases, recorded only the
predicted cases, and did the bookkeeping (CHANGELOG, issue status, `backlog.md`, progress).

## Items

| # | item(s) | outcome | commit | tests added | golden impact |
|---|---|---|---|---|---|
| 1 | B-001 FD queue merge | done: validated patch, `immediateWriteFrequencyDomain` searches the whole queue for (t, λ) | b2bd836 | gtest `OpticalOutputPort.FrequencyDomainWriteMergesBehindQueueTop` (fails before: 3 entries); golden `regress/b001-fd-multilambda-stale` (m = o = 0.5625 W at 1550 nm, was 0.25 W) | none on existing cases (as predicted) |
| 2 | B-144 + B-145 + B-147 bidir slots | done: next-free-slot convention; reader rule of the prototype kept; `upgrade_signal(net, device_writes_net)` (required parameter) swaps only when the device reads the net, extended from WG to COUPLER, PSHIFT, MZI and CROSSING-bi (roles of the uni variant: inputs read, outputs write); uni crossing outputs `AS_WRITER`; convention stated once in `parse_tree.h` | 6d6bf9c | 14 golden cases `regress/b144-*`, `b145-*`, `b147-*` (all bidir repros + `b145-upgrade-roles` for PSHIFT/COUPLER/MZI), hand values in the netlist headers, all matched | none on existing cases (as predicted; `regress/b031-mzi-bidir` unchanged) |
| 3 | B-146 subckt port nets via `nets_flat` | **stopped**: the fix works (hierarchical == flat) but changes `cir-json/subckt_tests` (`flat.json` `writers` of `ROOT/out` and `ROOT/X2/N00001`: 0 → 1); the table predicted no change on existing cases | not on `ws/engine`; ready: `P2-ready/b146` @ 8129020 | 5 golden cases on that branch (`regress/b146-x-shared-{net,delayed}(-flat)`, `err/b146-x-net-type-mismatch`) | see "Needs sign-off" below |
| 4 | B-052 positional args | done: CWSRC `wl power phi`, VLSRC/EVLSRC `file`, PCMCELL `emelt n tc ta`; the four asserts → `Error (<name>): too many positional arguments (N, expected at most M)` | aaa02b3 | 4 `regress/b052-*-positional` + 4 `err/b052-*-args` | none on existing cases (as predicted) |
| 5 | B-051 PCMCELL tc/ta | done: tc/ta required (presence check, any alias, `tc=0` valid); `PCMElement` default Ta = 1 (header); `OctaneCell`/`OctaneSegment`/`OctaneMatrix` take Tc/Ta as required parameters | 344d60b | `regress/b051-pcmcell-tc-zero`, `err/b051-pcmcell-no-{tc,ta}` | none (no case built a PCMCELL without tc/ta; `tb/pcm` unchanged) |
| 6 | B-060 validation | done: `PCMElement::check_parameters()` rejects n ≤ 0 and k ∉ [0, n], from the parser and from `start_of_simulation()`; model untouched | c22ff40 | gtests `PCMElementDeathTest.*` (2); `regress/b060-pcmcell-k-eq-n`, `err/b060-pcmcell-{n-negative,n-zero,k-negative,k-gt-n}` | new cases only (as predicted) |
| 7 | B-002 + B-004 | done: `sampled-time` rejected in the CLI, `SPECSConfig::verifyConfig()` and `OpticalOutputPort::applyConfig()` (configuration time, never per event); `SPECSConfig::simulation_mode_explicit` set by `-m`, the analyses only apply their default mode without it | c6414ce | gtest `OpticalOutputPortDeathTest.ApplyConfigRejectsSampledTime` + a `verifyConfig` death line; `regress/b004-mode-fd-tran`, `regress/b004-mode-td-op`, `regress/b004-mode-td-dc` | `opt/mode-sampled-time` → `expect: fail` (exit 1, clean error), re-recorded. `opt/mode-fd` **unchanged**: its netlist (`circuits/add_drop.cir`) is a `.dc`, whose default is already FD, so "runs in the requested mode" changes nothing; the new `b004-mode-fd-tran` (`-m fd` on a `.tran`) shows a changed mode instead. `tb/ac_add_drop-fd` unchanged |
| 8 | B-058 | done: `TimeMonitor` (`time_monitor.*`), its meson entry and includes, `--mon` and its setup code removed | 575209a | — | `cli/help`, `cli/no-args`: only the `--mon` line removed, re-recorded (as predicted) |
| 9 | B-037 | done: per-instance warning and its TODO removed; comments say what k/kp/t set (parse_element.cpp, directional_coupler.h, crow.h, generic_2x2_coupler.h); COUPLER section in `doc/netlist.md` (incl. the paper's κ = power ratio, `kp`) | 8e30077 | — | stdout loses the warning line, re-recorded in 21 cases: 15 existing cases that print it, `err/b034-coupler-*` ×2 (stdout ignored, stored with the line), and the P2 cases `b004-mode-*` ×3 and `b145-upgrade-roles` (`opt/mode-sampled-time`, in the commit message's list, was already an error case with an empty stdout after item 7). Verified: the old baseline with only that line removed compares equal to the new one with `--rtol 0 --atol 0 --print-slack 0` (VCD bytes of 4 pointer-ordered cases differ in `$var` id codes only) |

Status/progress commits: 7a99a64, 916b6f2, d92197e, 3e967f1, 5c9c910, 45bfa61, af07934, 0498866, 4083a49, 1029911,
5463c8e (+ this report). The gtest case was re-recorded with the new tests (B-001, B-060, B-002); `manifest.json`
`rerecords` has one entry per recording, each citing its item.

### Hand-checked values of the new bidirectional cases (item 2)
φ = 2π·2.2111·1e-4/1.55e-6 wraps to −2.1890 rad (WG `length=1e-4`, defaults); merger out = (in1+in2)/√2.

| case | expected = observed | before the fix |
|---|---|---|
| b144-bidir-source-first | out:0 = 1 W @ φ | out = 0 |
| b144-uni-writer-first / -second | b:0 = 1 mW @ −2.1890 (both orders) | first: b = 0 |
| b144-two-sources-one-net | a:0 = 1 mW, a:1 = 2 mW, rc 0 | E115 |
| b144-three-writers | E115, rc 1 (known limitation, `expect: fail`) | same |
| b145-upgrade-reader (-ref) | out = (5+4cos φ)/2 mW = 1.341 mW @ 2.4261 | 0.5 mW |
| b145-upgrade-reader-case2 (-ref) | out = 1.341 mW @ −0.5209 | same (worked) |
| b145-upgrade-roles | PSHIFT/COUPLER/MZI writing and reading the upgraded net: out_wps 4.5 mW, out_wdc 3.38 mW, out_wmzi 2.5 mW, out_r* 4.5 mW | out_w* = 0, out_r* = 2 mW |
| b147-crossing-out-bidir | e:0 = 1 mW @ φ (same as a k=0 coupler) | e = 0 |
| b147-crossing-out-explicit-wg / -splitter / -splitter-first | e:0 = 1 mW, z1 = z2 = 0.5 mW | wg: 0 |

Line-order permutations (implementer, fixed build): all repros give one result in every order tried (exhaustive for
the small ones, e.g. 720 orders of crossing-out-bidir; 50+ sampled orders for the upgrade cases), except the two
residuals listed under "Found, not done" (uni reader on a 2-writer net; the `upgrade_signal` assert).

## Needs sign-off: B-146 (item 3)
The decision ("flatten merges subckt port nets against `nets_flat`, hierarchical = flat") is implemented and
verified on `P2-ready/b146` (8129020, one commit on top of 4083a49; cherry-picks cleanly onto `ws/engine`): the two
`nets` lookups in `ParseTree::flatten()` use `nets_flat`; `a4-x-shared-net` (was E115) and `v7-x-shared-delayed` (was
last-write-wins, mid = 2 mW) give OP listings identical to their flat netlists (mid:0 = 1 mW, mid:1 = 1 mW resp.
2 mW); `a4-x-net-type-mismatch` gives "Could not reconcile nets: ROOT/X2/e and ROOT/n". Golden on that branch:
120/120 at `--atol 0` after re-recording `cir-json/subckt_tests`.
The one existing-case change: `cir-json/subckt_tests` `flat.json`, `writers` of `ROOT/out` and `ROOT/X2/N00001`
0 → 1. Each of these nets is written by one subckt instance and read by another; before the fix the second instance
overwrote the first one's counts, so 1 is the flat-equivalent count. `cir/subckt_tests` (stdout, VCD) and every
`bidirectional` flag are unchanged. The backlog's "byte-identical JSON" predates B-154 (0dcfda3), when the export
printed the readers count in both fields. It is a JSON-export correction, not a simulation change, and it follows
from the decision's "hierarchical = flat", but the P2 table said "none on existing cases", so per the rules it was
held back. To land: `git cherry-pick 8129020` onto `ws/engine`, then set B-146's status.

## Decisions taken (not covered by the decision record; none changes results beyond the decisions)
1. **Integration order 1, 4–9, 2** (3 stopped): the bidir implementer took longer and items 4–9 don't depend on
   it (B-147 lands with B-144; B-146 would come after B-144). One commit and one golden re-record per item, so every
   baseline change stays attributable.
2. **Docs**: the cheatsheet is a PDF without sources, so the user docs went into a new `doc/netlist.md` (COUPLER,
   CWSRC, VLSRC, EVLSRC, PCMCELL: nets, positional order, keywords with aliases, required ones, units), linked from
   `README.md`, `doc/mainpage.md` and the Doxyfile `INPUT`. It says it takes precedence over the PDF. Regenerating
   the PDF is left to P8 (B-080).
3. **Line order of the new cases**: all new cases check line order (no `unordered_streams`) except
   `regress/b004-mode-fd-tran` (runs `circuits/add_drop_pyspecs.cir`, already exempt as `cir/add_drop_pyspecs`).
   The bidir implementer had proposed `unordered_streams` for all 13 of its cases; I recorded them strict and checked
   that gcc, clang and gcc `-O0` builds give the same order (115/115 strict on each).
4. **B-144**: the reader rule is the prototype's (`(max(n_writers, 1) + 1) % 2`), which is how I read "readers read
   the other side"; the reader/writer counters in `upgrade_signal` are left untouched; three writers recorded as an
   `expect: fail` case with stdout ignored (the E115 text is on stdout and names drivers in creation order).
5. **B-051**: `OctaneCell` and `OctaneMatrix` lose their `meltEnergy = 10e-12`/`nStates = 63` defaults, because C++
   cannot have required parameters after defaulted ones; an old-style call is now a compile error instead of
   silently binding emelt to Tc. None of these classes is instantiated today (B-069). No physics value was chosen.
6. **B-060**: the same `check_parameters()` runs from the parser and from `PCMElement::start_of_simulation()` (C++
   users; it cannot affect `tb/pcm`, n = 63, k = 0); n = 0 keeps its existing "needs values for both Emelt and
   Nstates" message (checked first); no Emelt ≤ 0 check (not decided). Status: validation part fixed, model points
   1–4 deferred to PCM v2 (scout F-10).
7. **B-002**: also rejected in `OpticalOutputPort::applyConfig()` (once per port and phase) for API users who set a
   port config directly; `-m sampled-time` fails even with `-p`/`-t` (checked right after CLI parsing, like
   `-m bogus`). The `--help` text was **not** changed (it still lists `sampled-time`), because `cli/help` was not
   predicted to change in item 7.
8. **B-004**: with `.op`/`.dc`, an explicit `-m td` is kept but changes nothing except the printed and traced mode,
   because `runOPAnalysis()`/`runDCAnalysis()` put every port in NO_DELAY mode anyway (golden cases
   `b004-mode-td-op`/`-td-dc` record this). No error or warning was added for that combination (not decided).
9. **B-058**: also removed the two now-unused `time_monitor.h` includes of `src/tb/crow_tb.cpp` and
   `freqsweep_tb.cpp`.
10. **B-037**: also re-recorded `err/b034-coupler-k-gt-1`/`-kp-negative` (stdout ignored, but stored with the line).

## Found, not done
Bidirectional nets / parser:
- **`upgrade_signal` asserts `has_writer ^ has_reader`**, but a unidirectional net can already have one writer and
  several readers bound when a bidir device upgrades it. Default (assert-enabled) builds abort (rc 134), NDEBUG
  builds give the right value. Repro: `v7-upgrade-reader-case2.cir` with `cwsrc3` listed before `wg1`; 24 of 54 line
  orders of case2 and 12 of 24 of the `b145-upgrade-roles` reader sub-circuits abort (base too). Suggested fix:
  `assert(has_writer || has_reader)`. **For P4**: `regress/b145-upgrade-reader-case2` and `regress/b145-upgrade-roles`
  pass today because the pointer-ordered creation order (B-176) avoids the assert; name-ordered iteration may hit it.
- A uni **reader** on a net with 2 writers still reads a slot that depends on the creation order (B-144 second
  check): `cwsrc1 a; wg1 b a; splitter1 a z1 z2` gives z1 = 0 in 3 of 6 orders, 0.5 mW in the others.
- Three writers on one net still give E115 (`bidirectional()` only tests `m_writers_count == 2`, B-164).
- JSON `readers`/`writers` of nets upgraded to bidirectional are wrong (e.g. `a4-upgrade-reader`: `c` shows 2 writers
  for one forward writer, `e` 1 writer for two).
- Copy-pasted comment "Bidirectional phase-shifter needs only optical nets as bidir" in the MZI bi variant.
- For P3: `CrossingBi` is never instantiated (B-159: backward light into a crossing output never comes out of its
  input); the crossing creates energy (B-032: `xtalk=-10`, 1 mW into each input gives 1.732 mW per output); the
  `b147-*` cases will change with B-032/B-033, and the `.tran` bidir cases have all-zero probe VCDs (B-048), so they
  will be re-recorded by P3.
CLI / modes:
- `--help` still lists `-m sampled-time`; the `-m td` message still says "by default".
- The 17 testbenches that set `simulation_mode = EVENT_DRIVEN` ignore an explicit `-m` silently (e.g. `-t wg -m fd`);
  the decision only covers netlist directives.
- An `.op` netlist that never advances time writes no VCD at all (probably related to B-005).
- `OpticalSignal::operator+=`/`-=` branch on the global `simulation_mode` and print to `cerr` in TD when two
  wavelengths are summed (an event-path print; not triggered in any run here).
Sources / PCM:
- CWSRC: a phase given while the power is 0 is dropped ("setting phase has no effect on 0"), so `cwsrc a wl 0 phi`
  followed by `power=` keeps phase 0.
- VLSRC/EVLSRC: a positional file plus `values=` mixes append (file) and replace (list) semantics; undocumented.
- The VALUES-list code accepts `None` entries, but a netlist cannot write one.
- PCMCELL: Emelt is not range-checked (negative accepted; `emelt=0` doubles as "not given"); the existing "PCM Cell
  needs values for both Emelt and Nstates" and "Unknown keyword: X" messages lack the `Error (<name>)` prefix; a
  default-constructed C++ `PCMElement` is still opaque at k = 0 (Tc = 0, as decided).
- KiCad `utils/specs.kicad_sym`: the PCM symbol uses `levels=63` (not a PCMCELL keyword) and lacks emelt/tc/ta.
Coupler:
- Keywords come from a `std::map`: if several of `k`/`kp`/`t` are given, the alphabetically last key wins, not the
  last written; `k` and `K` are separate keys and both apply (documented in `doc/netlist.md`, not changed).
- `loss` is not range-checked (a negative value is gain).
- `OctaneSegment::m_coupling_through_row/col` have no comment saying they are power ratios.
- Bare `k` was not deprecated in favour of `kf`/`kp` (optional in the issue, not decided).
Other:
- B-001 design follow-up (from the issue): order equal-time queue entries FIFO and drop the inverted
  `OpticalSignal::operator<`; the fix, like `delayedWriteEventDriven`, mutates a non-top heap element.
- B-011: the OP listing shows one wavelength per net, so `regress/b001-fd-multilambda-stale` only checks 1550 nm.
- Pre-existing `-Wstrict-aliasing` warnings at `src/specs.cpp:365-368` (`(int&)` casts in `sc_trace`).
- The new `.op` cases depend on the OP listing order, which is pointer-ordered (B-016): stable in gcc, clang and
  gcc `-O0` today, but an ASan build may reorder them (as P1 found for 7 older cases).

## Final results
HEAD before this report: 5463c8e. Fresh builds with the prebuilt deps (`specs-deps-next`): `/tmp/P2-fgcc` (gcc 11.5,
Meson defaults, asserts on) and `/tmp/P2-fclang` (clang 19.1.7).
- `meson test`: gcc 12/12 OK, clang 12/12 OK (unit, 9 tb, golden-harness, golden).
- `python3 -I tests/golden/run_golden.py check --specs <b>/specs --unit-tests <b>/src/test/unit_tests --baseline
  tests/golden/baseline -j 8 --atol 0` (strict order and coverage by default since P1): gcc and clang **115 passed
  (1 with notes), 0 failed, 0 new, 0 missing, 0 skipped** (114 `specs` cases + gtest with 18 tests, all PASS). The
  note is a pre-existing `unordered_streams` case.
- Line order of the new cases also checked in a gcc `-O0` build (`/tmp/P2-debug`) and the earlier clang build
  (`/tmp/P2-clang`): 115/115 strict.
- `P2-ready/b146` (not on `ws/engine`): 120/120 at `--atol 0` with its own baseline.
- Compiler warnings: none in changed code (remaining: SystemC `SC_HAS_PROCESS` deprecations under clang,
  `src/specs.cpp:365-368` strict-aliasing, `value_list_source.cpp:40` unused variable, generated parser/scanner).
- Golden baseline: 81 → 115 `specs` cases (+34 new: 1 B-001, 8 B-052, 3 B-051, 5 B-060, 3 B-004, 14 B-144/145/147);
  re-recorded existing cases: `opt/mode-sampled-time` (B-002), `cli/help` and `cli/no-args` (B-058), 17 existing
  coupler cases (B-037), gtest (B-001, B-060, B-002).

## Branches and worktrees
`ws/engine` holds the phase. Implementer branches (all integrated by cherry-pick, can be deleted): `P2-wip/bidir`,
`P2-wip/pcm`, `P2-wip/cli`, `P2-wip/docs`, `P2-wip/bidir-b146` (superseded by `P2-ready/b146`). Keep
`P2-ready/b146` until B-146 is decided. The `/tmp/P2-wt/*` worktrees were removed (`git worktree remove`).
