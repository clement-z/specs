# SPECS — project instructions for coding agents

(Claude Code reads this file through `CLAUDE.md`, which imports it.)

Scalable Photonic Event-driven Circuit Simulator: C++20 on SystemC 3.0.2, a flex/bison SPICE-like netlist
parser, and a Python wrapper (`pyspecs/`). Paper: `doc/papers/zrounba2023-specs-ecoc.pdf` (ECOC 2023).
Orientation: `discover/ONBOARDING.md`; deep dives in `discover/0*.md`; known issues in `backlog.md`.

## Build & run
- Deps: SystemC 3.0.2 (**built as C++20**), taywee/args 6.4.7 and googletest come from `subprojects/*.wrap`:
  `meson setup` downloads them (offline: `meson subprojects download` first) and builds SystemC with its CMake
  build. An installed SystemC/args only on request: `-Dforce_fallback_for= -Dcmake_prefix_path=<prefix>`
  (or `--wrap-mode=nofallback`, which errors if none is usable).
- Build (Meson, release by default): `meson setup build && meson compile -C build`
  - Options (`meson.options`): `native` (`-march=native`, default off: B-183), `signal_ids` (debug ids of
    optical signals, default off), `old_tb`, `spx`, `tests`, `golden`, `docs`.
    `libspx.so`: `meson compile -C build spx`. Docs: `ninja -C build doc`. Decisions: `discover/meson/DECISIONS.md`.
  - Asserts are on by default (`b_ndebug=false`); `-Db_ndebug=true` gives the same outputs.
  - **Don't run `ninja clang-format`** (Meson's automatic target): it reformats every file.
- pixi route (`pixi.toml`; the plain route stays the reference for users). Agents use it when `pixi` is on the PATH:
  same toolchain in every worktree, and one ccache and wrap cache shared by all of them.
  - **A checkout or worktree without a `.pixi/` directory: run `pixi install --locked` first** (seconds when pixi's
    package cache is warm; `--locked` fails on an outdated `pixi.lock` instead of re-solving it).
  - `pixi run test [gcc|clang|asan-gcc|asan-clang|gcc-native|clang-native]` (configure + build + `meson test`;
    `pixi run build-native` = the `gcc-native` build), `pixi run golden [toolchain]`,
    or any command prefixed with `pixi run` (e.g. `pixi run meson test -C build-pixi/gcc`). Build dirs are
    `build-pixi/<toolchain>`; never mix them with `build/`. Details: README "Development environment".
- Run: `build/specs -f circuits/add_drop.cir -o traces/out` (needs a `traces/` dir in the cwd;
  `.include` paths resolve relative to the including file, then the cwd). Legacy testbenches: `build/specs -t wg` (list them with `specs -p`).

## Testing — golden outputs are the safety net
- Any change that could affect behaviour must pass:
  `python3 -I tests/golden/run_golden.py check --specs build/specs --unit-tests build/src/test/unit_tests --baseline tests/golden/baseline -j 8`
  (also registered as `meson test -C build --suite golden`; `meson test -C build` runs everything).
  Line order, NEW/MISSING cases and the gtest case are all checked by default: `--allow-new` while a new
  case is not recorded yet, `--no-unit-tests` to skip gtest explicitly; an `--only` that matches nothing exits 2.
  Output order is name-based (F-01), so sanitizer and other-compiler builds pass with strict order too.
- Sanitizers: `meson setup <b> -Db_sanitize=address,undefined -Db_lundef=false --buildtype=debug` (gcc and clang
  both work locally). `meson test` then applies `tests/sanitizers/lsan.supp`, which suppresses only
  parse-error-exit flex buffers and SystemC-thread-stack allocations; any other leak fails (CI job is blocking).
  Running `run_golden.py` directly on a sanitizer build needs the same suppressions
  (`LSAN_OPTIONS=suppressions=$PWD/tests/sanitizers/lsan.supp:print_suppressions=0`), else error cases fail on leak reports.
- Bit-identical outputs (`--rtol 0 --atol 0 --print-slack 0`): the baseline comes from a portable (non-native) gcc
  release build (B-183), and gcc release builds reproduce it exactly (system gcc 11, pixi gcc 15). clang and
  debug/sanitizer builds differ in 6 cases at one sweep point (<= 1e-14 relative), native builds in 28 (<= 1e-12);
  all pass the default tolerances and `--atol 0`. For bit-identity before/after a change, compare like with like.
- If a change **intentionally** alters outputs (a bug fix), re-record with
  `run_golden.py record ... --out tests/golden/baseline --only '<ids>' --note "<why, backlog ID>"`, inspect the diff
  (`compare`), and commit the new baseline **in the same commit** as the fix.
- Unit tests: gtest in `src/test/` (`meson test --suite unit`; list new files in `src/test/meson.build`).
  SystemC can only elaborate once per process, so wrap each test body in
  `SPECS_TEST_HEADER()` / `SPECS_TEST_FOOTER()` (fork-per-test, `src/test/utils.h`).
- Legacy `src/tb/*_tb.cpp` testbenches have no assertions; golden outputs are what check them.

## Code style (match the surrounding code)
- **Never bulk-format** (`ninja clang-format`, `clang-format -i` on whole files): 104/108 files don't follow
  `.clang-format`. Reformat only lines you change, in the local style. Control statements mostly use Allman braces.
- `#pragma once`. Project includes are quoted and relative to `src/` (`#include "devices/waveguide.h"`); `<systemc.h>`.
- Classes PascalCase; Uni/Bi variants share an `XxxBase`. Members `m_` **with units in the name**
  (`m_length_cm`, `m_attenuation_dB_cm`). Ports `p_in`/`p_out`, `p0_in`/`p1_out`. Reactions `on_<port>_changed()`.
- Process style (F-15): a stateless device reaction is an `SC_METHOD` (`dont_initialize()`; constants computed in
  `start_of_simulation()`, `--vci` prints in a one-shot `print_initialization()` thread created only with `--vci`).
  `SC_THREAD` only where `wait()` with a time is needed (sources, detector tick). The kernel runs a delta cycle's
  methods before its threads, so converting a process changes its order relative to the others: check strict
  golden and `stats-compare` (`tests/golden/README.md`).
  Method case is mixed (camelCase/snake_case), so follow the class you're in. `typedef` over `using`.
- Units: SI in netlists, **cm internally** for waveguide lengths, dB/cm; optical field is √W.
- Errors: `std::cerr << ...; exit(1);` is the existing pattern. Don't add new `assert`s for user-input validation.
- No new globals; no new `using namespace std;` in headers; no printing in per-event hot paths.

## Adding a netlist element
Lexer prefix in `src/parser/parser.l` → token/rule in `parser.y` → `XxxElement` in `parse_element.h` +
`::create` in `parse_element.cpp` (`INSTANTIATE_AND_CONNECT_{UNI,BI}`) → `devices/alldevices.h` → docs
(cheatsheet) → new `.cpp` files go in the explicit lists in `src/**/meson.build` → a golden case in `tests/golden/cases.json` → a unit test if it has logic of its own.

## Physics & numerics
Changes to device equations, units, tolerances (abstol 1e-8 / reltol 1e-4), time resolution, or event-scheduling
semantics change simulation results. **They need human sign-off**: open/annotate a backlog item tagged
`needs-human` instead of changing behaviour silently.

## Git
- Integration branch `dev`, `--no-ff` merges to `master`. Short lowercase imperative subjects
  (`fix(B-012): ...`, `build: ...`, `doc: ...`).
- Never `git push` unless the maintainer explicitly asks.
- Root `.gitignore` ignores `*.cir` (except `circuits/*.cir`; `tests/golden/` re-includes its own) and `*.vcd`
  repo-wide: use `git add -f` for other new netlists (e.g. `backlog/repro/`).
