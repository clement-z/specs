# B-172: yyerror always exits; exit codes are inconsistent (1/2/3/134/139/E549)
severity: low | category: cli | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.y:945-980, src/parser/parser.l:180-187, src/main.cpp:172-174, src/main.cpp:392-396, src/parser/parse_tree.cpp:337-340, src/parser/parse_tree.h:825
## Problem
`yyerror` calls `exit(1)`: one error per run, no recovery, and the `parsing_result != 0` branches are dead. Exit
codes:
- 1: parse and semantic errors;
- 3: lexer errors (`exit(yylerror(...))`);
- 2: "third writer" (parse_tree.h:825);
- 134: asserts (Debug);
- 139: segfault (B-149);
- 1: SystemC E549/E115.

pyspecs and tools cannot classify failures.
## Evidence
Observed in this review: rc 3 for `.ic` with `exit(yylerror)`, 134 in the B-148 table, 139 in B-149, 1 for
E549/E115.
## Expected
A documented exit-code policy (e.g. 1 = netlist error, 2 = elaboration error, 3 = runtime error).
## Suggested fix
Decide the policy. Make `yyerror` return and use bison error recovery (or count errors and stop at the end of the
line). Delete the dead `parsing_result` branches.
## Golden impact
expect=fail cases record the exit code (cli/*, cir/subckt_test, ...) and would change if codes change.
## Related
B-148, B-155.

- Discovery seeds: S3.24 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-35.
## Verification
Code reading plus the runs listed.
