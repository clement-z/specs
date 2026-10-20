# B-113: Parser grammar/lexer diagnostics: 2 useless nonterminals (3 rules) and a flex "dangerous trailing context"
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.y:121,622,683, src/parser/parser.l:483
## Problem
Every build prints (bison 3.7.4 / flex 2.6.4):
`parser.y: warning: 2 nonterminals useless in grammar`, `3 rules useless`, `parser.y:622 net.oa.bidir`, `parser.y:683 net.edigital`, and `parser.l:483: warning, dangerous trailing context` (twice).
`net.oa.bidir` (2 alternatives) and `net.edigital` (1) are never referenced by any production. Whether to delete them or wire them in (EDIGITAL nets look like unfinished feature work) is a decision. The flex warning is for the rule `{ALPHA_PLUS}{ALPHA_PLUS_NUM}*/{WS}*= {` (variable-length head and trailing context). The two parts are disjoint character classes, so the match is correct, but flex compiles the whole scanner with a state-buffer ("reject buffer") path for it; this is the code behind clang's `-Wmisleading-indentation` at the generated `scanner.cpp:1790` (`if(!yyg->yy_state_buf)` followed by an indented `if`), and it slows every token match. Survives the migration (the generated files are the same).
## Evidence
`/tmp/ws-review-ninja.log` lines for steps [10] and [11]; parser.y has `%type <s_ptr> net.oa.bidir` (121) and the rule at 622, `net.edigital` at 683; `grep -n "net.oa.bidir\|net.edigital" src/parser/parser.y` -> only those lines.
## Expected
Warning-free generation, or an explicit `%expect`/comment for the intentional ones.
## Suggested fix
Delete both unreachable rules (and the `%type` entry) or connect them; for the flex rule, replace the trailing context by a one-token lookahead handled in the action (e.g. match the identifier, then peek for `=` with `yyless`/a start condition), which also drops the REJECT-style state buffer. Needs a decision and a parse-regression check (golden `cir/*`, `cli/*`).
## Golden impact
Should be none; verify with the full golden `check`.
## Related
B-109, B-110.

- Review ids: F-A7-09.
## Verification
Generator output read from the log; unreachability by grep; the clang warning location read from the generated `scanner.cpp`.
