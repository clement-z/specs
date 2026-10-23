# B-030: `SC_HAS_PROCESS` is a deprecated no-op in SystemC 3.0 (36 sites)
severity: low | category: style/maintainability | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: fixed in 9f31c11 (sprint 2026-10c PG): the 38 uses removed; clang build without the deprecation warnings
location: src/optical_output_port.cpp:24 and 35 other `SC_HAS_PROCESS` sites (`grep -rn SC_HAS_PROCESS src`)
## Problem
In SystemC 3.0 it expands to a `static_assert` on a `[[deprecated]]` variable. clang warns once per use. GCC 11
does not, because the header is `-isystem`.
## Evidence
`clang++ -std=c++20 -fsyntax-only src/optical_output_port.cpp` → `warning: 'sc_has_process_used' is deprecated`.
## Expected
A clean build.
## Suggested fix
Delete the uses: `SC_THREAD` uses `decltype(*this)` in 3.0. This drops SystemC 2.3 compatibility (CLAUDE.md
requires 3.0.1, but check the Makefile/CI). The alternative is defining `SC_ALLOW_DEPRECATED_IEEE_API`.
## Golden impact
None.
## Related
none. The seed's stack-memory claim is not filed: thread stacks are committed lazily (max RSS 8-12 MB measured on
the repro runs). SC_METHOD conversion is a separate design topic.

- Discovery seeds: S1.32, S4.17 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-30.
## Verification
Compiled with clang and read the installed sc_module.h:412-418.
