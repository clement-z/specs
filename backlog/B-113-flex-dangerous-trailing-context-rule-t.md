# B-113: Flex "dangerous trailing context" rule for `T_ASSIGN_ID` (parser.l:483)
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.l:483
## Problem
Every build prints (bison 3.7.4 / flex 2.6.4):
`parser.y: warning: 2 nonterminals useless in grammar`, `3 rules useless`, `parser.y:622 net.oa.bidir`, `parser.y:683 net.edigital`, and `parser.l:483: warning, dangerous trailing context` (twice).
`net.oa.bidir` (2 alternatives) and `net.edigital` (1) are never referenced by any production. Whether to delete them or wire them in (EDIGITAL nets look like unfinished feature work) is a decision. The flex warning is for the rule `{ALPHA_PLUS}{ALPHA_PLUS_NUM}*/{WS}*= {` (variable-length head and trailing context). The two parts are disjoint character classes, so the match is correct, but flex compiles the whole scanner with a state-buffer ("reject buffer") path for it; this is the code behind clang's `-Wmisleading-indentation` at the generated `scanner.cpp:1790` (`if(!yyg->yy_state_buf)` followed by an indented `if`), and it slows every token match. Survives the migration (the generated files are the same).

Scope note: the useless-nonterminal part is split out as F-Z-01 (easy-unattended); `%option noyy_top_state` is B-109 and `has_reader` is B-112. This issue keeps only the flex trailing-context rule.
## Evidence
`/tmp/ws-review-ninja.log` lines for steps [10] and [11]; parser.y has `%type <s_ptr> net.oa.bidir` (121) and the rule at 622, `net.edigital` at 683; `grep -n "net.oa.bidir\|net.edigital" src/parser/parser.y` -> only those lines.

Additional evidence (independent report F-A4-37, A4 reviewer):

The scratch build log (`ninja specs`) shows exactly these warnings.
## Expected
Warning-free generation, or an explicit `%expect`/comment for the intentional ones.
## Suggested fix
Delete both unreachable rules (and the `%type` entry) or connect them; for the flex rule, replace the trailing context by a one-token lookahead handled in the action (e.g. match the identifier, then peek for `=` with `yyless`/a start condition), which also drops the REJECT-style state buffer. Needs a decision and a parse-regression check (golden `cir/*`, `cli/*`).
## Golden impact
Should be none; verify with the full golden `check`.
## Related
B-109, B-110.

- Discovery seeds: S3.27, S4.16 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A4-37 (A4).
- Review ids: F-A7-09, F-A4-37.
## Verification
Generator output read from the log; unreachability by grep; the clang warning location read from the generated `scanner.cpp`.
