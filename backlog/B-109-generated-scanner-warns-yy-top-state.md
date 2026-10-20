# B-109: Generated scanner warns `yy_top_state defined but not used`; one flex option removes it
severity: low | category: build | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parser.l:30
## Problem
`%option stack` makes flex emit `yy_top_state()`; the grammar only uses `yy_push_state`/`yy_pop_state` (parser.l:244,263,309,334,473). GCC (`-Wunused-function`) and clang both warn on every build of `scanner.cpp`. The warning survives the Meson migration (same generated file, `warning_level=2`).
## Evidence
`/tmp/ws-review-ninja.log`: `generated/parser/scanner.cpp:3247:16: warning: 'int yy_top_state(yyscan_t)' defined but not used [-Wunused-function]`.
Adding `%option noyy_top_state` after `%option stack` (parser.l:30) and diffing the regenerated scanner (ignoring `#line`) removes exactly the prototype (2 lines) and the 6-line function body, nothing else (`backlog/repro/a7-flex-top-state.sh`).
## Expected
No warning from the generated scanner under GCC.
## Suggested fix
```
src/parser/parser.l:30   %option stack
+                        %option noyy_top_state
```
## Golden impact
none (generated code differs only by the removed unused function).
## Related
B-113 (remaining flex/bison diagnostics), B-110.

- Review ids: F-A7-02.
## Verification
Regenerated `scanner.cpp` with and without the option (flex 2.6.4), diffed; compiled the patched scanner with `g++ -fsyntax-only -Wall -Wextra`: 0 warnings (was 1). Safe: an unused static function is removed.

**Independent second check (V2, of F-A7-02): CONFIRMED.** Method: applied `%option noyy_top_state` after `%option stack` on scratch copy, rebuilt (ninja -j2) with flex 2.6.4, compared build logs, golden check bit-exact (--atol 0 --rtol 0)

Baseline log has `scanner.cpp:3247:16: warning: 'int yy_top_state(yyscan_t)' defined but not used [-Wunused-function]`. After the fix flex regenerates scanner.cpp without `yy_top_state` (grep count 0; diff of old/new generated scanner ignoring `#line` = the 2-line prototype + 6-line body, nothing else, same as a7-flex-top-state.sh) and the warning is gone; the only other output of the rebuild is the pre-existing parse_tree.cpp:217 warning (different finding). The two flex "dangerous trailing context" diagnostics are unchanged (line 483 -> 484). Golden: 44/44 pass, also at atol=rtol=0. No design choice.
