# B-104: CHANGELOG is stale: names the tag `v0.1.0` (it is `v0.1`) and omits all work since v0.1 except the CMake switch
severity: low | category: docs | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: CHANGELOG.md:1-12
## Problem
The tag is `v0.1` (ff9b8bb, 2023-09-30). There are 34 commits since; the unreleased section lists only the CMake switch (and says the Makefile "should still function", which B-120 shows false).
## Evidence
`git tag -n` -> `v0.1 Public release`; `git log --oneline v0.1..HEAD | wc -l` -> 34.
## Expected
Notable user-visible changes recorded.
## Suggested fix
Rename `## v0.1.0` to `## v0.1`; under `[unreleased]` add (from git log): multiple netlist files via repeated `-f` (13c2d1a, e95498f); command-line options take priority over netlist `.options` (1b0780d); trace file reports SPECS settings (ce5bd6f); PWR power-meter element (0e4b33a); photodetector `noise_bypass`/`frequency` parameters (f1d8aa8, 4b1c60c); PCM device update and parser parameters (3650565); bidirectional-device and parser fixes (05026d5, dd122cf, c8450b9); SystemC 3.0.1 / C++20 (50ae939); `libspx` shared library target and gtest unit tests (1f08662, 252a7f4); ECOC 2023 paper (26bf92c); golden-output regression harness (241ec0a). Apply after the Meson commit (PLAN.md step 5 also edits this section) and replace the false "Makefile should still function" bullet.

Reclassified `needs-design` after the second check: the finding gives a list of changes, not the exact text, and the wording/categories are the maintainer's call; the section also has to be rebased on the Meson commit. Only the `## v0.1.0` -> `## v0.1` heading rename is mechanical. Correction: C++20 came with the CMake refactor (bd5c8f8); 50ae939 only bumps the SystemC submodule and install.sh.
## Golden impact
none.
## Related
B-120, B-116.

- Discovery seeds: S5.25 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-29.
## Verification
`git log`, `git tag`; entries cross-checked against the commit subjects.

**Independent second check (V5, of F-A6-29): ADJUST.** Method: git tag / git log / commit-subject cross-check; read CHANGELOG.md

Facts check out: tag is `v0.1` (ff9b8bb, "Public release"), CHANGELOG says `## v0.1.0`; commits 13c2d1a, e95498f, 1b0780d, ce5bd6f, 0e4b33a (PWR element exists in parser.l:453), f1d8aa8, 4b1c60c, 3650565, 05026d5, dd122cf, c8450b9, 50ae939, 1f08662, 252a7f4, 26bf92c, 241ec0a all exist with matching subjects (v0.1..a9e5799 is 34 commits as stated; v0.1..HEAD is 36 now because of two later review commits). BUT this is not mechanical and should not be `easy-unattended`: the finding gives a bullet list as prose with commit hashes, not the exact text to insert. A fixer has to write the wording, choose the category headings (New / Fixed), decide whether to include hashes and replace the "Makefile should still function" bullet (whose falsity rests on B-120, not on this finding). It must also wait for the Meson commit (the Meson plan edits the same section), so the base text is not fixed yet. One inaccuracy in the proposed content: "SystemC 3.0.1 / C++20 (50ae939)": 50ae939 only bumps the SystemC submodule and install.sh; the C++20 switch came with the CMake refactor (bd5c8f8). Suggested re-class: only the trivial `v0.1.0` -> `v0.1` rename is mechanical; the rest should be `needs-review` (maintainer wording) or the finding should carry the exact final text. Location: the file has 13 lines, not 1-12.

**Orchestrator:** Reclassified easy-unattended -> needs-design (content is not mechanical).
