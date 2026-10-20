# SPECS — project instructions

Scalable Photonic Event-driven Circuit Simulator: C++20 on SystemC 3.0.1, a flex/bison SPICE-like netlist
parser, and a Python wrapper (`pyspecs/`). Paper: `doc/papers/zrounba2023-specs-ecoc.pdf` (ECOC 2023).
Orientation: `discover/ONBOARDING.md`; deep dives in `discover/0*.md`; known issues in `backlog.md`.

## Build & run
- Deps: SystemC 3.0.1 **built as C++20** and taywee/args. On this machine they are prebuilt in
  `~/.local/opt/specs-deps`; otherwise use the submodules (`git submodule update --init --recursive`).
- Build (Meson, release by default):
  `meson setup build -Dcmake_prefix_path=$HOME/.local/opt/specs-deps && meson compile -C build`
  - Options (`meson.options`): `native` (`-march=native`, default on), `old_tb`, `spx`, `tests`, `golden`, `docs`.
    `libspx.so`: `meson compile -C build spx`. Docs: `ninja -C build doc`. Decisions: `discover/meson/DECISIONS.md`.
  - Asserts are on by default (`b_ndebug=false`). The golden baseline was recorded with `-DNDEBUG`; both pass.
  - Without the prebuilt deps: `./install.sh`, or plain `meson setup build` (builds SystemC from the submodule).
  - **Don't run `ninja clang-format`** (Meson's automatic target): it reformats every file.
- Run: `build/specs -f circuits/add_drop.cir -o traces/out` (needs a `traces/` dir in the cwd;
  `.include` paths resolve relative to the cwd). Legacy testbenches: `build/specs -t wg` (list them with `specs -p`).

## Testing — golden outputs are the safety net
- Any change that could affect behaviour must pass:
  `python3 -I tests/golden/run_golden.py check --specs build/specs --unit-tests build/src/test/unit_tests --baseline tests/golden/baseline -j 8`
  (also registered as `meson test -C build --suite golden`; `meson test -C build` runs everything).
- If a change **intentionally** alters outputs (a bug fix), re-record with
  `run_golden.py record ... --out tests/golden/baseline --note "<why, backlog ID>"`, inspect the diff
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
  (`m_length_cm`, `m_attenuation_dB_cm`). Ports `p_in`/`p_out`, `p0_in`/`p1_out`. Threads `on_<port>_changed()`.
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
- Root `.gitignore` ignores `*.cir` and `*.vcd` repo-wide: use `git add -f` for new example/repro netlists.
