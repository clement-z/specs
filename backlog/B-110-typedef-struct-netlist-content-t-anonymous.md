# B-110: `typedef struct {...} netlist_content_t` is an anonymous struct with default member initialiser (clang warning, C++20 P1766R1)
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parser.l:48-56
## Problem
An unnamed struct that has a default member initialiser (`FILE *fp = nullptr;`) and non-trivial members (`std::string`) and is named only through a `typedef` is no longer a valid "typedef name for linkage" in C++20 (P1766R1). Clang 19 already warns (`-Wnon-c-typedef-for-linkage`) and newer GCC/clang may turn it into an error. Needed for the clang job planned in PLAN.md:102. Survives the migration.
## Evidence
```
$ clang++ -fsyntax-only -std=gnu++20 -Wall -Wextra ... generated/parser/scanner.cpp
parser.l:48:15: warning: anonymous non-C-compatible type given name for linkage purposes by typedef declaration; add a tag name here [-Wnon-c-typedef-for-linkage]
```
parser.l:48-56: `typedef struct { enum type {NETLIST_FILE, STRING} type; string description; YY_BUFFER_STATE buf; FILE *fp = nullptr; } netlist_content_t;`
## Expected
Named struct, no warning.
## Suggested fix
parser.l:48 `typedef struct {` -> `struct netlist_content_t {`; parser.l:56 `} netlist_content_t;` -> `};`. All uses (`netlist_content_t::STRING`, `deque<netlist_content_t>`, parser.l:108-159) are unchanged.
## Golden impact
none.
## Related
B-109 (same file, can be one commit "fix(parser): scanner warnings").

- Review ids: F-A7-03.
## Verification
Applied both edits to a scratch copy, regenerated, compiled with `g++ -fsyntax-only` and `clang++ -fsyntax-only`: no errors, the clang warning is gone (only the flex-skeleton `-Wmisleading-indentation` remains, see B-113).
