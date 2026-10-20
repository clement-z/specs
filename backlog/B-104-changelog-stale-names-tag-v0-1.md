# B-104: CHANGELOG is stale: names the tag `v0.1.0` (it is `v0.1`) and omits all work since v0.1 except the CMake switch
severity: low | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: CHANGELOG.md:1-12
## Problem
The tag is `v0.1` (ff9b8bb, 2023-09-30). There are 34 commits since; the unreleased section lists only the CMake switch (and says the Makefile "should still function", which B-120 shows false).
## Evidence
`git tag -n` -> `v0.1 Public release`; `git log --oneline v0.1..HEAD | wc -l` -> 34.
## Expected
Notable user-visible changes recorded.
## Suggested fix
Rename `## v0.1.0` to `## v0.1`; under `[unreleased]` add (from git log): multiple netlist files via repeated `-f` (13c2d1a, e95498f); command-line options take priority over netlist `.options` (1b0780d); trace file reports SPECS settings (ce5bd6f); PWR power-meter element (0e4b33a); photodetector `noise_bypass`/`frequency` parameters (f1d8aa8, 4b1c60c); PCM device update and parser parameters (3650565); bidirectional-device and parser fixes (05026d5, dd122cf, c8450b9); SystemC 3.0.1 / C++20 (50ae939); `libspx` shared library target and gtest unit tests (1f08662, 252a7f4); ECOC 2023 paper (26bf92c); golden-output regression harness (241ec0a). Apply after the Meson commit (PLAN.md step 5 also edits this section) and replace the false "Makefile should still function" bullet.
## Golden impact
none.
## Related
B-120, B-116.

- Discovery seeds: S5.25 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-29.
## Verification
`git log`, `git tag`; entries cross-checked against the commit subjects.
