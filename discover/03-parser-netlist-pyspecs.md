# 03 — Parser, netlist language, pyspecs/utils (discovery probe 3/5)

Scope: `src/parser/`, `circuits/`, `doc/mainpage.md`, `doc/resources/cheatsheet.pdf`, `pyspecs/`, `utils/`.
Method: read the grammar and actions in full, then **checked behaviour against a real binary**. I built SystemC 3.0.1, args and `specs` (CMake, `BUILD_TB=OFF`, Debug) **out of tree** under `/tmp/specs-scratch/`, so the repo is untouched. The binary is `/tmp/specs-scratch/specs-build/specs` and the ~45 probe netlists are in `/tmp/specs-scratch/nets/`.
Toolchain on host: flex 2.6.4, bison 3.7.4, g++ (C++20), Python 3.9.21 (pyspecs deps **not** installed).

---

## 1. Summary

- **Front end.** A reentrant flex scanner (`parser.l`, 581 lines) feeds a pure bison LALR parser (`parser.y`, 980 lines). Bison reports **no S/R or R/R conflicts**. The language is SPICE-like and case-insensitive: one element or directive per line. The device type comes from the **name prefix** (`WG*`, `COUPLER*`, `CWSRC*`, ...).
- **Pipeline.** Grammar actions build a `ParseTree` holding `ParseElement*`, a `ParseNet` map, analyses, directives and subcircuits. `ParseTree::build_circuit()` then does two things:
  1. It flattens subcircuits by **re-lexing their captured text** in a fresh scanner.
  2. It elaborates SystemC modules net-by-net, bidirectional nets first, through per-device `create()` → `instantiate_and_connect_{uni,bi}`.
- **Values.** Numbers are C-style only (**no SI suffixes**). Expressions are `+ - * / ^`, parentheses, `{var}` references, `"strings"` and `[lists]`. Two parameter keywords exist: `.assign` and `.param` (aliases). Variables are not scoped across subcircuits.
- **Error handling is fail-fast.** Every error calls `exit()`, so there is no recovery and only one error per run. Exit codes vary: 1 for parse errors, 3 for lexer errors, 134 for `assert` aborts and 139 for a segfault. Locations are good for top-level files. They are **wrong for included files and absent for subcircuit bodies and for all semantic errors**, which happen at elaboration time.
- **Verified high-impact bugs:**
  - **(a)** A net shared by two subcircuit instances loses its reader/writer counts. This gives SystemC `E115 multiple drivers` on a circuit whose flat equivalent simulates fine.
  - **(b)** CWSRC positional args are shifted by one, so `args[0]` is silently ignored.
  - **(c)** `.include` inside `.subckt` corrupts the body.
  - **(d)** Subcircuits cannot see top-level `.param` values or top-level `.subckt` definitions.
  - **(e)** `.nodeset` can never parse.
  - **(f)** `X1` with no arguments segfaults.
  - **(g)** Several user errors hit `assert()` and abort.
- **Examples.** 4 of the 9 `circuits/*.cir` fail: 3 use an obsolete pre-release syntax and `subckt_test.cir` has typos. `paul_3ring_puf.cir` needs a CSV that is not in the repo. Only `add_drop*.cir` and `subckt_tests.cir` run.
- **Docs.** The cheatsheet is the only user documentation of the syntax. It is wrong on `.dc` syntax, the photodiode prefix, comma-separated kwargs, phase units and merger kwargs. `doc/mainpage.md` is a stub that links a missing QUICKSTART.
- **Python.** `pyspecs` is a thin subprocess wrapper plus a VCD→pandas parser. It has no packaging and undeclared deps, and contains a likely-wrong sweep detection (`'b01'` vs VCD `b1`). `draw_graph_from_json.py` is duplicated byte-for-byte in `utils/`. It consumes `--json` output, which has a `writers` = `readers` bug.

---

## 2. Netlist language reference (as implemented)

### 2.1 Lexical rules (`src/parser/parser.l`)

| Topic | Rule (file:line) |
|---|---|
| Case | `%option caseless` (:27). Keywords and prefixes are case-insensitive. **Element and subckt names are uppercased** (`parse_tree.cpp:368-369,394`). **Net names are case-sensitive**: only the `ROOT/` prefix is uppercased (`parse_tree.h:651-666`). Verified: `In` and `in` are two different nets. Variable names are case-sensitive. Kwarg names are uppercased at create time. |
| Comments | `*` in column 0 is a full-line comment (:358). `;` starts an inline comment anywhere (:353). |
| Continuation | `+` must be in **column 0** of the next line (:534). An indented `+` is a syntax error (verified). It does not apply inside captured `.subckt` bodies; these are re-lexed later, so continuation works there. |
| Line end | `\r?\n` (:21). A last line without EOL is accepted, but error messages then show the previous line (:516). |
| Whitespace | `WS [ \xC2\xA0\t]` (:15): the NBSP bytes are matched *individually*. |
| Numbers | `{D}+` → `T_INT` and `D*.D+[eE±D+]` → `T_NUM` (:8, :340-351). Leading-dot is OK (`.5`). **No SI suffixes**: `10u` is a syntax error (verified). There is no `1.` form. Sign comes from unary minus in the grammar. |
| Strings | `"..."` with no escapes and no newline (:262-272, :333). |
| Identifiers | `[A-Za-z_][A-Za-z0-9_]*` → `T_STR`. The same pattern followed by optional WS and `=` → `T_ASSIGN_ID` (:483; flex warns "dangerous trailing context"). |
| Element names | Anchored at line start, prefix plus **at least 1** `[A-Za-z0-9_]` (:363-457). A bare `WG` or `cwsrc` is **not** an element: it is a syntax error. |
| Ignored | `.title ...` and `.end` (KiCad compatibility, :526-531). |
| Fatal | `.ic` → `exit(1)` with **no message** (:469). Stray `.ends` → error (:478). Unknown char → `exit(3)` (:556). |

### 2.2 Grammar skeleton (`src/parser/parser.y`)

```
input   := { line '\n' }
line    := ε | element | analysis | directive | subcircuit | local_assignment
element := PREFIX<id> net... {pos_arg} {kw '=' expr}        # positional args must precede kwargs
         | X<id> {net_or_name} {kw '=' expr}                # last positional = subckt name
pos_arg := INT | NUM | "str" | {var} | [list] | '{' expr '}' # "delimited" value
expr    := ['+'|'-'] term {('+'|'-') term} ; term := fact {('*'|'/') fact}
fact    := pow {'^' pow}   (left-assoc: 2^3^2 = 64, verified) ; pow := constant | '(' expr ')'
constant:= INT | NUM | "str" | '{' NAME '}' | [list]
list    := '[' [expr] {',' [expr]} ']'   (empty slot = None; "[]" → [None], verified)
net     := NAME | INT (→ "N%05d") | '/' NAME | '/' INT | net '-' NAME | "_" (fresh anonymous net)
```

- **Expression gotcha.** Inside `{ }`, bare names are illegal: `{b*2}` is a syntax error (verified). Write `{{b}*2}` as a positional arg, or `length={b}*2` as a kwarg.
- **No implicit None.** The lexer never emits `T_NONE`, so `None` is a syntax error (verified). The VLSRC error text still says "or None".
- **Arithmetic** always yields DOUBLE. `Variable` (`parse_tree.h:29-279`) supports DOUBLE, COMPLEX (unreachable from syntax), INTEGER, BOOLEAN (unreachable), STRING, LIST and NONE.
- **Globals:** `pi`, `c` (`parse_tree.cpp:980-984`).
- **Net typing** happens on first use: OANALOG or EANALOG (EDIGITAL is unreachable). A type mismatch is a parse error. Reader/writer counts are accumulated per net. The `&` suffix marks an explicit bidirectional optical net (`parser.y:609-620`).

### 2.3 Elements (`parser.y:248-392` → `parse_element.cpp`)

| Prefix | Nets (o=optical, e=electrical) | Positional args | Kwargs (case-insens.) | Device / dir |
|---|---|---|---|---|
| `WG` | o_in o_out | L[m] neff ng att[dB/cm] D | NEFF NG L\|LENGTH ATT D | Waveguide{Uni,Bi}. A self-loop is rejected. |
| `COUPLER` | o_in1 o_in2 o_out1 o_out2 | k(field) loss | K\|KF\|KFIELD, KP\|KPOW\|KPOWER, T(through field), LOSS | DirectionalCoupler, bidir. Prints "field" warning. |
| `MERGER` | o_in1 o_in2 o_out | il | IL\|INSERTION_LOSS | Merger, uni |
| `SPLITTER` | o_in o_out1 o_out2 | ratio il | IL\|INSERTION_LOSS, SPLITTING_RATIO\|RATIO | Splitter, uni |
| `PSHIFT` | o_in o_out e_in | sens att (assert ≤2) | ATTENUATION\|ATT, SENSITIVITY\|GAIN\|G | PhaseShifter, bidir on optical nets |
| `MZI` | o_in1 o_in2 o_out1 o_out2 e_in | sens (assert ≤1) | SENSITIVITY LENGTH LENGTH_REF NEFF NG ATT IL_DC IL_PHASESHIFTER | MZIActive, bidir |
| `CROSSING` | o_in1 o_in2 o_out1 o_out2 | att xtalk (assert ≤2) | ATTENUATION\|ATT, CROSSTALK\|XTALK | Crossing; declared bidir but **always instantiated uni** |
| `CWSRC` | o_out | **args[0] ignored**, wl, power (assert ≤3) | WL\|WAVELENGTH, POWER, PHI (rad, applied last) | CWSource |
| `VLSRC` | o_out | (dead code) | VALUES=`[[t,P,wl],…]` or `"file"`; FILE/SCALE → "not implemented" | VLSource |
| `EVLSRC` | e_out | (dead code) | VALUES=`[[t,V],…]` or `"file"` | EVLSource |
| `PROBE` | o_in | p mod phase wl (bools) | POWER\|POW\|P, MAGNITUDE\|MAG\|MODULUS\|MOD\|E, PHASE\|PHI, WAVELENGTH\|WL | Probe (all on by default) |
| `MLPROBE` | o_in | `[wl,…]` | WAVELENGTHS\|WL | MLambdaProbe |
| `PDET` | o_in e_out | ts R | SAMPLING_TIME\|TS, R\|RESPONSIVITY\|GAIN, NOISE_BYPASS\|NB, FREQUENCY\|FOP | Detector, uni |
| `PCMCELL` | o_in o_out | Emelt N (assert ≤2; Tc/Ta branches unreachable) | MELT_ENERGY\|EMELT\|E_MELT, N\|NSTATES\|NLEVELS\|N_STATES\|N_LEVELS, K\|INITIAL_STATE, SP\|TC\|T_C, EP\|TA\|T_A, TANH_COEF; Emelt and N required | PCMElement |
| `PWR` | o_in | none (assert 0) | none | PowerMeter |
| `X` | n1..nN | last = subckt name | subckt params | flattened (`XElement::create` exits if reached) |

An unknown kwarg produces `Unknown keyword: X` at **elaboration** time, with no file or line.

### 2.4 Directives and analyses

| Syntax | Behaviour (file:line) |
|---|---|
| `.assign a=expr b=expr …` / `.param …` | Sets local variables in source order; later lines can use `{a}` (`parser.y:186-194`). Re-assignment overwrites. |
| `.options kw=val …` | TRACEALL, ABSTOL, RELTOL, TS\|TIMESCALE (int enum, unchecked), RESOLUTION, TEST_VARIABLE (prints). Anything else → exit (`parse_directive.cpp:18-48`). |
| `.include path` | Path runs to WS or `;`; quotes are *not* stripped. Resolved **relative to CWD**. Lexed by pushing a buffer (`parser.l:243-260`). |
| `.subckt NAME p1 p2 … [kw=default …]` … `.ends` | The body is captured verbatim, with nested `.subckt` counting (`parser.l:274-331`). It is re-parsed at flatten time per instance (`parse_tree.cpp:303-354`). `.ends NAME` is a **syntax error** (verified). Analyses and directives inside a body are dropped with a warning. |
| `.op` | `CW_OPERATING_POINT`, frequency-domain (`parse_analysis.cpp:23`). Args are silently ignored. |
| `.dc /ELEM/ATTR start stop step` | CWSRC only. ATTR is WL\|WAVELENGTH\|LAMBDA, P\|POW\|POWER or F\|FREQ\|FREQUENCY. One sweep only. The element must be top-level (`/X1/CW1` is not addressable). `stop` is excluded (`general_utils.h:30-33`). Fewer than 3 points is rejected (`parse_analysis.h:59-66`). A bare `.dc` → assert abort (verified). |
| `.tran [T]` | `TIME_DOMAIN`. Two args → assert abort (verified). Any kwarg → exit. |
| `.nodeset /net/attr=val` | **Never parses**: `attr=` lexes as `T_ASSIGN_ID` while the grammar expects `T_STR` (`parser.y:774`; verified). `create()` exits "disabled" anyway (`parse_directive.cpp:65-67`). |
| `.save` | Token exists (`parser.l:461`) but no rule → syntax error (verified). |
| `.ic` | Silent `exit(1)` from the lexer (verified). |
| (none) | Only one analysis is allowed (`parse_tree.cpp:379-384`). Having no analysis runs and prints "Simulated 0 s" without a warning. |

**CLI coupling** (`main.cpp:382-391`):
- `-f` can repeat; the files are concatenated into one ROOT tree.
- `--reltol`, `--abstol` and `-R` are appended as a **footer** of `.options` lines, so they override the netlist.
- `--json FILE` dumps the *flattened* tree.
- `-n` stops after elaboration.

---

## 3. Parse → elaboration pipeline

1. `main.cpp:382-391`: `sc_main` with `-f` → `build_circuit(pt, files, footer)` (`main.cpp:143`).
2. `parser.l:147-178`, `yy_add_content_from_file`: queues `[header string, FILE buffer, footer string]` per file in a **global** `deque netlist_content_fifo` (:58). `main.cpp:152` adds the CLI footer. `yy_load_next_buf` (:99) switches buffers and sets the filename used in messages.
3. `main.cpp:166`: `yyparse(scanner, &pt)`. `%initial-action` sets the global `cur_pt` (`parser.y:165-169`). The actions `register_element`, `register_analysis`, `register_directive` and `register_subcircuit` are at `parse_tree.cpp:356-415`. Nets are created via `get_or_create_net` (`parse_tree.h:597-630`) with type and r/w counting (`parser.y:594-695`).
4. At EOF, `parser.l:539-550` pops the buffer, then `yy_load_next_buf` moves to the next queued buffer or terminates.
5. `main.cpp:176`: `pt.print()` dumps the **pre-flatten** tree.
6. `parse_tree.cpp:688`, `ParseTree::build_circuit()`:
   - `flatten()` (:819-978). For each `XElement`: `find_subcircuit` in the *defining* tree (:846). A new `ParseTree(name, subckt, kwargs)` (:303) re-runs flex and bison on the stored body text. Recursive `flatten()`. Port nets are renamed/merged (:864-919). Sub-elements are spliced in.
   - Bidirectional nets first (:702-752), then unidirectional (:755-804). It walks nets → unbuilt elements → `ParseElement::create(pt_helper)` (`parse_element.cpp`). `create()` makes signals (`create_signals`, `parse_tree.cpp:147`) and calls `INSTANTIATE` → `instantiate_and_connect_uni/bi`, which `new`s the device class from `src/devices/*` and binds ports with `connect_uni/connect_bi` (`parse_tree.h:804-931`). Kwargs are applied **after** construction by poking members (`obj->m_neff = ...`).
   - Name-collision check: SystemC renames duplicates and the code detects that (:714-720; verified for `wg1`/`WG1`).
   - `directives[i]->create()` (writes `specsGlobalConfig`), then `analyses[0]->create()` (`parse_analysis.cpp`). Modules and signals are registered in `specsGlobalConfig` (:811-816).
7. `main.cpp:397-405`: optional JSON export (`ParseTree::to_json`, `parse_tree.cpp:578`). `main.cpp:413` calls `specsGlobalConfig.runAnalysis()`.

Build: the CMake target `specs_parser` OBJECT lib (`src/parser/CMakeLists.txt`) uses `bison_target`/`flex_target` into `${build}/generated/parser`. The Makefile path generates into `build/parser`. The parser `.cpp` files are **excluded from the library** in both systems (`CMakeLists.txt:23`, `Makefile:27`), so `libspecs`/`spx` cannot parse netlists.

---

## 4. Example circuits (all run with the scratch binary, CWD containing `circuits/`)

| File | What it shows | Features used | Result (verified) |
|---|---|---|---|
| `add_drop.cir` | Add-drop ring filter, wavelength sweep | `.assign` with `{}` refs and expressions, inline `.subckt test` with defaults, `X` with kwargs, integer net `0`, COUPLER `k=`, WG kwargs, `.options`, `.dc /cwsrc1/WL …`; commented-out old `.dc WL(cwsrc1)` lines | rc=0. Sweeps 1535…1564 nm = **30 pts**; the 1565 endpoint is dropped. Writes VCD. |
| `add_drop_pyspecs.cir` | Same ring, time-domain pulse; used by the notebook | `vlsrc values=[[t,P,wl],…]`, integer nets, 3 probes, `traceall=1`, `.tran 3e-9` | rc=0, "Simulated 3 ns", VCD ~400 kB |
| `add_drop_sub.cir` | Library: `add_drop_filter` subckt (in drop out add) | `.subckt` with 6 params, `.assign` inside the body using global `{pi}` | Has no top-level content; runs 0 s with SystemC W571 warning. Meant for `.include`. |
| `common_definitions.def` | Shared `.assign` constants | `.assign`, `;` comments | **Not referenced anywhere** (orphan). |
| `subckt_tests.cir` | Subcircuit stress test | `.include`, nested `.subckt test` defined inside `.subckt test` (shadowing), int port `1`, `X`→`X` direct connection, kwargs override, VLSRC list with empty slots and negative power, `.tran` without duration | rc=0, simulated ~4.16 ns. Note: its X-X net `out` is hit by the flatten count bug; harmless here because it is unidirectional. |
| `subckt_test.cir` | Minimal subckt | `.subckt AAA 1 2 len=3` | **Fails** at line 5: bare `cwsrc` is not an element name. The body also uses a bare `WG`, and `pow=` is not a CWSRC kwarg. |
| `paul_3ring_puf.cir` | 3-ring PUF built from `add_drop_filter` instances | multi-assign lines, parenthesised exprs, string vars, `.include circuits/…` (CWD-relative), SPLITTER, MERGER, MLPROBE positional list, PDET with `_` net, 6 X instances, `resolution=` | Parses and elaborates, then **fails** with "Error: Cannot open the file." because `paul_puf_bitstream_158b_3wl.csv` is not in the repo and the message does not name the file. |
| `double_ring.cir`, `triple_ring.cir`, `paul_simple_ring.cir` | Ring resonators driven by a bitstream source | **Pre-release syntax**: `one`, `clk`, `bsrc`, bare `dc`/`wg`/`pdet`, SI suffix `350.85u`, `*tracefile`/`*tracenet` | **All fail** at line 4. Present unchanged since the "Initial release" commit `ff9b8bb`; never matched the current grammar. |

Not covered by any example: MZI, CROSSING, PCMCELL, PWR, EVLSRC, PSHIFT, `&` bidir nets, `.op`.

---

## 5. Docs vs implementation mismatches

`doc/mainpage.md` contains only "Introduction TODO" plus links to `readme.html` and `quickstart.html`. `doc/Doxyfile:832-835` lists `QUICKSTART.md` as INPUT, but **that file does not exist**. The README defers to the online docs (not checked; no network). The only syntax reference is `doc/resources/cheatsheet.pdf` (Inkscape, 2023-06):

| Cheatsheet says | Implementation | Verified |
|---|---|---|
| `.dc WL(cwsrc1) wl0 wl1 dwl` | `.dc /cwsrc1/WL wl0 wl1 dwl` (`parser.y:786-807`) | syntax error at `WL` |
| Photodiode `pd 1 2 r=val ts=val` | Prefix is `PDET`; `pd1` is not an element | syntax error |
| `pshift1 1 2 3 att=val, gain=val` | Kwargs are whitespace-separated; `,` is illegal | syntax error at `,` |
| CWSRC "phi … in degrees" | `polar(|E|, phase)`, i.e. **radians** (`cw_source.h:43-48`) | code read |
| MERGER "il … ; loss is the insertion loss" | Only `IL`/`INSERTION_LOSS`; `loss` → Unknown keyword | code read |
| PROBE "1 is the output terminal" | It is the input | code read |
| `.options … ` / `+ abstol=…` | Works only if `+` is in column 0 | indented `+` → error |
| `.op` "ignoring all VLSRC" | Not checked by the parser | unknown |
| Not documented | MZI, CROSSING, PCMCELL, PWR, `X`/`.subckt`, `.assign`/`.param`, `.include`, `{}` expressions, `_` nets, `&` bidir, COUPLER `kp`/`t`, `.options resolution`/`ts`, VLSRC `values="file"`, `.tran` default duration | — |

**KiCad library** (`utils/specs.kicad_sym`, v20220914 = KiCad 7) vs lexer:
- `ADD_DROP_FILTER` (Ref `ADROP`), `ALL_PASS_FILTER` (Ref `APASS`) and `PCM` (Ref `PCM`, needs `PCMCELL`) have **no lexer prefix**.
- The PCM template uses `levels=63`, which is not a valid kwarg, and omits the required Emelt.
- The `ELECTRIC_V_SOURCE_LIST` template uses 3-column values, but EVLSRC needs 2.
- The `*_SOURCE_FILE` templates use `values=\` instead of a quoted filename.
- KiCad auto-names nets like `Net-(WG1-Pad2)`, which is a syntax error at `(` (verified). `/in` works but becomes `ROOT//in`.

---

## 6. pyspecs & utils

| File | What it does | Inputs / outputs | Issues |
|---|---|---|---|
| `pyspecs/specs.py` | `simulate()` builds argv and runs `subprocess.Popen([dir+"specs", …])`, timing the run | `-f`, `-o`, `--abstol`, `--reltol`, or a raw `custom_arguments` dict (1-char key → `-k`, else `--key`). Returns `(ms, (rc, stdout_bytes))`. | Imports numpy, pandas and matplotlib without using them. Default `-o traces/delete_me.vcd` produces `…vcd.vcd` because SystemC appends `.vcd` (:19). `simulator_directory + "specs"` needs a trailing slash (:232). Passing both netlist and custom args duplicates flags (:223-229). Prints "SPECS had an error" whenever `verbose` is set (:119-120). |
| `pyspecs/specsparse.py` | `parse_vcd_dataframe(vcd)` → `(df_probes, df_pdet)`. Probe and pdet scopes are detected by child signal names. `find_by_time`, `verify_wl_sweep`. | SPECS VCD with top scope `SystemC` and a `SystemC.analysis_type` var | **`check_wl_sweep` compares to `'b01'`, but the VCD contains `b1`** for CW_SWEEP (:265; enum `specs.h:75-80`; seen in `add_drop.vcd`), so sweeps may be read as TD. `has_wl = False` should be `True` (:76), so wavelength is re-added per probe. A skipped first signal leaves `current_signal_df` undefined or stale (:71-84). `fillna(method='ffill')` is deprecated in pandas 2.1 and removed in 3.0 (:104,146). Timescale parsing assumes `"<n> <unit>"`. |
| `pyspecs/draw_graph_from_json.py` (= `utils/draw_graph_from_json.py`, **identical**) | Renders `specs --json` output to a Graphviz `sfdp` graph: nets as HTML-table nodes, elements as nodes with nets/args/kwargs | JSON from `ParseTree::to_json` | It shows wrong `writers` because the JSON bug copies `readers` (`parse_tree.cpp:573`). `main()` raises NameError when no argv is given (:126-133). `shorten` truncates without an ellipsis. `render(outfile=)` needs graphviz-py ≥0.19 (assumed). |
| `pyspecs/pyspecs_example.ipynb` | Demo: simulate `add_drop_pyspecs.cir`, parse the VCD, plot probe power, `find_by_time` | Expects the binary at `../specs` (Makefile layout) | `install.sh` and CMake build `build/specs`, so the path is stale. Kernel metadata says Python 3.8.10. Outputs are committed. |
| `pyspecs/__init__.py` | empty | — | No `requirements.txt` or `pyproject`. Deps implied: numpy, pandas, matplotlib, **pyDigitalWaveTools 1.2**, graphviz; utils also needs scipy. f-strings require Python ≥3.6. |
| `utils/analyze_detector_trace.py` | Offline coherent pulse-merging analysis of a photodetector CSV (`P (W)`, `tau (s)`, `phi (rad)`, `t (s)`, `lambda (m)`, `id`). Pickles `NonIntersectingPulses.obj`/`Pout.obj` and saves `Pout.png` in CWD. | `detector_trace.txt` (default) or argv files | **No producer** of that CSV exists in current `src/` (grep finds nothing), so it is likely orphaned. `compare_Pout_vecs` is broken (text-mode pickle, undefined names). Needs scipy. |
| `utils/openwave.sh` | Opens a VCD in gtkwave, letting the user pick a `.gtkw` from a directory | `$1` trace, `$2` config dir | `#!/bin/env bash` is not portable. `${#configs}` (:34) is the string length, not the array length, so the bounds check is wrong. |
| `utils/specs.kicad_sym` | KiCad 7 symbol library with `Sim.Device=SPICE`, `Sim.Params` templates | — | See §5. No X/PWR/EVLSRC-proper symbols. |

---

## 7. Unknowns

- **pyspecs is unrun.** pandas, matplotlib, pyDigitalWaveTools, graphviz and scipy are not installed and there is no network. It is unknown whether pyDigitalWaveTools normalises `b1` to `b01`, which decides whether the sweep-detection bug is real.
- **Missing VCDs for `.op`.** `.op` runs (`t_cw_pos3`) printed "tracing timescale unit set: … .vcd" but **no VCD file was created**. This is engine-side and out of scope; flagged for probe(s) covering `specs.cpp`.
- **CWSRC positional order.** The intended order is undocumented; the code reads `[?, wl, power, phase]`.
- **Online docs.** Whether the online docs (demo.zrounba.fr) define a newer syntax than the cheatsheet. Not reachable.
- **Makefile build.** Not exercised: it uses `-std=c++17` and is what CI uses. Only the CMake/C++20 build was tested.
- **KiCad export.** The real KiCad 7 SPICE netlist output (`.title`, net naming, `Sim.Params` placement) was not exercised end to end.
- **`double_ring` family.** Whether these files are meant to be kept (historical) or ported. Git shows them unchanged since `ff9b8bb` (2023-09-30).

---

## 8. Seeds for backlog

- [severity: high] src/parser/parse_tree.cpp:899 — Flatten merges port nets against `nets` (the pre-flatten parent map) instead of `nets_flat`. X-element nets are never registered in the parent either (`parser.y:513-541` uses `net_name_base`). (why: a net joining ≥2 subckt instances keeps only the last instance's r/w counts and skips type checks; `x1 in mid s` + `x2 mid in2 r` → SystemC E115 "more than one driver", while the flat equivalent works.)
- [severity: high] src/parser/parse_element.cpp:646-652 — CWSRC positional args are off by one: `args[0]` is ignored, `args[3]` is unreachable behind `assert(args.size()<=3)` at :637. (why: `cwsrc1 in 7 1.3e-6 0.5` silently gives 1300 nm and 0.5 W, so the first value is dropped.)
- [severity: high] src/parser/parse_tree.h:682-697 + src/parser/parse_tree.cpp:846 — Variable and subcircuit lookup do not walk parent scopes. (why: a top-level `.param L` used in a subckt fails with "Variable not found: L", and a subckt instantiating another top-level subckt fails with "Subcircuit definition not found"; both verified; this blocks hierarchical designs.)
- [severity: high] src/parser/parser.l:243 — `.include` is active in the `SUBCKT_DEFINITION` state, so it is executed at definition time and `BEGIN(INITIAL)` breaks body capture. (why: verified syntax error; including device libraries inside a subckt is a normal use case.)
- [severity: high] doc/resources/cheatsheet.pdf — The only syntax doc is wrong on `.dc`, `pd`, comma kwargs, phi units and merger `loss`, and omits MZI, CROSSING, PCMCELL, PWR, subckt, param and include. Doxyfile/mainpage reference a missing QUICKSTART. (why: new users hit syntax errors on the documented examples.)
- [severity: med] src/parser/parse_analysis.cpp:34,87; parse_directive.cpp:21; parse_element.cpp:396,492,584,637,705,858,1001,1084,1157,1202 — User-input validation is done with `assert`. (why: `.tran 1e-9 2e-9` and bare `.dc` abort with core dumps, rc 134, verified; with NDEBUG the input is silently accepted or ignored.)
- [severity: med] src/parser/parse_tree.cpp:841 — `xelem->args.back()` is called on empty args. (why: a line `X1` segfaults, rc 139, verified.)
- [severity: med] src/parser/parser.y:774 — `.nodeset /net/attr=val` is unparsable because `attr` lexes as `T_ASSIGN_ID`, and `NODESETDirective::create` exits anyway (parse_directive.cpp:65). `.ic` exits silently (parser.l:469). `.save` has a token but no rule (parser.l:461). (why: dead or half features that fail confusingly; fix them or reject with a clear message.)
- [severity: med] src/parser/parse_tree.cpp:322-330; parser.l:247-256 — Error locations are wrong:
  - subckt-body errors print `(:N)`: empty filename and a body-relative line;
  - errors in included files print the *parent* filename with the child's line;
  - all elaboration-time errors (unknown kwarg, missing var, bad net count) have no location.
  (why: debugging large netlists; store file:line on each ParseElement and feed filename and line offset into sub-parses.)
- [severity: med] src/parser/parser.l:248 — `.include` resolves relative to CWD, and the `fopen`'d FILE is never closed. (why: `paul_3ring_puf.cir` only works from the repo root; it should resolve relative to the including file.)
- [severity: med] circuits/double_ring.cir:4, triple_ring.cir:4, paul_simple_ring.cir:4, subckt_test.cir:5, paul_3ring_puf.cir:18 — 4/9 examples fail to parse and 1 needs a missing CSV. (why: examples are the de-facto spec; port or delete them, and add a CI smoke test running every `circuits/*.cir` with `-n`.)
- [severity: med] src/parser/parse_tree.cpp:573 — JSON `"writers"` emits `m_readers_count`; `Variable::to_json` (parse_tree.h:129) does not escape strings; `"subcircuits":"TODO"` (parse_tree.cpp:645). (why: `--json` is the machine interface used by draw_graph and probably the web demo.)
- [severity: med] src/parser/parse_element.cpp:539-540,590 — Crossing outputs are bound `AS_READER`, and the crossing is always instantiated unidirectional despite `DECLARE_BIDIR_ELEMENT`. (why: wrong connection bookkeeping; bidirectional crossings are impossible.)
- [severity: med] src/parser/parse_element.cpp:714,867 — VLSRC/EVLSRC positional filename is dead code (`args.size()>1` vs `assert(<=1)`); PCMCELL Tc/Ta positional args are unreachable (:1202 vs :1215-1218). (why: the same off-by-one family as CWSRC; audit every positional-arg block.)
- [severity: med] pyspecs/specsparse.py:265 — Sweep detection compares to `'b01'`, but SPECS VCD writes `b1`. Also `has_wl=False` (:76) and pandas-3-removed `fillna(method=)` (:104,146). (why: DC sweeps are likely misparsed as time-domain; breaks on current pandas.)
- [severity: med] pyspecs/ (no manifest) — No requirements or pyproject. The notebook expects `../specs` while CMake builds `build/specs`. `draw_graph_from_json.py` is duplicated in pyspecs/ and utils/. (why: Python side is not installable or reproducible.)
- [severity: med] src/parser/parser.l:340-351 — No SI suffixes (`u n p f k meg`). (why: SPICE users expect `10u`; the legacy examples use them; this is the most common syntax error.)
- [severity: med] utils/specs.kicad_sym — The ADROP/APASS/PCM refs have no lexer rule, the templates are wrong, and KiCad default net names `Net-(…)` fail to lex. (why: the advertised KiCad flow is broken out of the box.)
- [severity: low] src/parser/parse_analysis.h:59-66 + src/utils/general_utils.h:30-33 — DC sweep excludes `stop`, accumulates FP error, and rejects 2-point sweeps with the message "at least 2 point". (why: SPICE `.dc` includes the endpoint; add_drop gives 30 pts instead of 31.)
- [severity: low] src/parser/parser.l:294-312 — `.ends NAME` (standard SPICE) is a syntax error. (why: compatibility with existing SPICE habits and tools.)
- [severity: low] src/parser/parser.l:133; parser.y:188-193,204; main.cpp:146; parse_tree.cpp:342-352,406 — Leaks:
  - `cstr` is leaked because `yy_scan_string` already copies the string;
  - `.assign` pairs are never deleted;
  - `ParserState` is leaked;
  - cleared directives and analyses are not deleted;
  - sub-tree subcircuits are never freed.
  (why: valgrind reports 1.45 kB definitely lost on `subckt_tests.cir`; bounded, but noise for future valgrind runs.)
- [severity: low] src/parser/parser.l:58,101,189,191 — The global deque, static `last_file`, global `cur_pt` and static counters make the "reentrant" scanner non-reentrant. (why: blocks parsing from the library or in parallel; prerequisite for an API and the parser library.)
- [severity: low] CMakeLists.txt:23 / Makefile:27 — Parser sources are excluded from `libspecs`/`spx`. (why: library users cannot load netlists.)
- [severity: low] src/parser/parser.y:945-980 — `yyerror` always calls `exit(1)`: no error recovery and a single error per run. The `parsing_result` branches in `main.cpp:172,392` are dead. Exit codes are inconsistent (1/3/134/139). (why: tooling and pyspecs cannot classify failures.)
- [severity: low] src/parser/parse_tree.h:651-666 — Net names are case-sensitive while everything else is case-insensitive. (why: `In` vs `in` silently creates two nets, verified; a classic SPICE pitfall; at least warn.)
- [severity: low] src/parser/parser.y:907,917 — `^` is left-associative (2^3^2=64); `[]` yields `[None]`; `None` is unusable (T_NONE never lexed) though the error text at parse_element.cpp:743 advertises it. (why: expression-semantics surprises.)
- [severity: low] src/parser/parser.l:15,483,534; parser.y:622,683 — The NBSP bytes in the WS class are matched individually; flex warns "dangerous trailing context"; continuation needs column-0 `+`; bison reports useless nonterminals `net.oa.bidir`/`net.edigital`. (why: build-warning hygiene and lexer robustness.)
- [severity: low] src/parser/parse_element.cpp:378,475,1058,1247; parse_directive.cpp:38 — Copy-pasted "coupling power for DC" warnings on PSHIFT and MZI, debug `cout`s, and a duplicate TRACEALL branch. (why: noisy output; cleanup.)
- [severity: low] src/parser/parse_tree.cpp:808 / main.cpp:413 — No warning when the netlist has no analysis (it runs "Simulated 0 s"). (why: user confusion.)
- [severity: low] src/devices/value_list_source.h:59, electrical_value_list_source.h:56 — "Cannot open the file." does not name the file. (why: `paul_3ring_puf` failure is opaque.)
- [severity: low] .gitignore (`*.cir`) — New circuit files are ignored by git; the existing ones were force-added. (why: contributors' example netlists silently go uncommitted.)
- [severity: low] src/main.cpp:65-141 — `do_circuit()` is dead code (the old single-file path with stdin support). (why: dead code; delete it.)
- [severity: low] utils/openwave.sh:1,34; utils/analyze_detector_trace.py:287-294 — The shebang is not portable, the bounds check is wrong, `compare_Pout_vecs` is broken, and the script reads a CSV that no current code produces. (why: bit-rotted helper scripts; fix or archive.)
