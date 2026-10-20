# B-177: Delete the useless grammar nonterminals `net.oa.bidir` and `net.edigital` (bison warnings)
severity: low | category: build | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 2cf960d
location: src/parser/parser.y:119, src/parser/parser.y:121, src/parser/parser.y:622-632, src/parser/parser.y:683-695
## Problem
Every build prints `parser.y: warning: 2 nonterminals useless in grammar` and `3 rules useless`
(`parser.y:622 net.oa.bidir`, `parser.y:683 net.edigital`). No production references either nonterminal, so they
are unreachable and only add warning noise. Split out of B-113 / B-113 so that this behaviour-neutral part can
be fixed alone; the flex "dangerous trailing context" part stays in B-113 (needs-design).
## Evidence
`grep -n "net.oa.bidir\|net.edigital" src/parser/parser.y` matches only the `%type` declarations (:119, :121) and
the rule definitions (:622, :683). Bison 3.7.4 output in the build log (`/tmp/ws-review-ninja.log`, steps [10]/[11]).
## Expected
Bison generates the parser without "useless" warnings.
## Suggested fix
In `src/parser/parser.y`: drop `net.edigital` from the `%type` list at :119, delete the `%type` line at :121
(`net.oa.bidir`), delete the rules at :622-632 (`net.oa.bidir`) and :683-695 (`net.edigital`), and make sure the
preceding rule (`net.eanalog`) still ends with `;`. If EDIGITAL nets are wanted later, the rule can be restored from
history; it is unreachable today.
## Golden impact
None: the language accepted by the grammar is unchanged (the rules are unreachable). Verified by the A4 reviewer on
a scratch build together with the other B-113 changes: golden check passes.
## Related
B-113 (flex trailing-context rule, needs-design), B-109 (`yy_top_state`), B-112 (`has_reader`).

- Discovery seeds: S3.27, S4.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-Z-01.
## Verification
Reviewers A4 and A7 both confirmed the warnings and that the nonterminals are unreferenced. A4 applied this change
(together with the other B-113 items) on a scratch build: bison warnings gone, golden check passes.

**Independent second check (V8, of F-Z-01): CONFIRMED.** Method: applied the deletions (type list :119, :121, rules :622-632 and :683-694, kept the closing `;`), rebuilt, golden --atol 0; ran bison 3.7.4 on the old and new parser.y and compared the generated tables

The bison warnings "2 nonterminals useless" / "3 rules useless" disappear; only the flex "dangerous trailing context" warning (B-113) remains. The language is unchanged: yytable, yycheck, yypact, yydefact, yypgoto, yydefgoto, yyr1, yyr2 and yystos are byte-identical, with 199 states in both. Golden: 44 pass, no notes.
