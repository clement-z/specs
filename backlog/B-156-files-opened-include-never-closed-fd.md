# B-156: Files opened by `.include` are never closed (fd exhaustion, misleading "not found")
severity: med | category: memory/UB | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in af92c25
location: src/parser/parser.l:248, src/parser/parser.l:539-550
## Problem
The include rule `fopen`s into `yyin` and pushes a buffer. `yypop_buffer_state` frees the buffer but never closes
the FILE. With the usual 1024-fd limit, the 1021st include fails, and the error claims the file is not found. After
B-150, every subckt instance re-reads its includes, so 1000 instances of a subckt with an include will hit this.
## Evidence
`backlog/repro/a4-include-fd-leak.sh <specs> <repo>` (1100 top-level includes, `ulimit -n 1024`): rc 1,
`Error: included file not found "circuits/common_definitions.def"`, `includes done: 1020`.
## Expected
All 1100 includes succeed.
## Suggested fix
In `<INITIAL><<EOF>>` (parser.l:539):
```
+    FILE *popped_file = YY_CURRENT_BUFFER ? YY_CURRENT_BUFFER->yy_input_file : nullptr;
     yypop_buffer_state(yyscanner);
+    // a buffer below the popped one means it was pushed by .include: close its file
+    if (YY_CURRENT_BUFFER && popped_file)
+        fclose(popped_file);
```
Main-file buffers are alone on the stack (they are switched, not pushed) and are still closed by
`yy_load_next_buf`. String buffers have a NULL file. Optional: add `strerror(errno)` to the message at :252.
## Golden impact
None. Verified: with this patch, the only golden notes were "stdout: line order differs (reordered)" on
cir/subckt_tests and opt/vci. That is the known pointer-ordered OP printout, sensitive to heap layout; it is an
informational note, not a failure.
## Related
B-150, B-143, B-170.

- Discovery seeds: S3.10 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-15.
## Verification
Patched scratch build: 1100 includes under `ulimit -n 1024` give rc 0. `subckt_tests.cir` and the
include-in-subckt repro still run.

**Independent second check (V8, of F-A4-15): CONFIRMED.** Method: applied the suggested <<EOF>> patch, rebuilt, golden --atol 0, ran a4-include-fd-leak.sh base vs patched, strace of open/close on nested includes, ran subckt_tests and include-in-subckt

Base: rc 1, `Error: included file not found "circuits/common_definitions.def"`, 1020 includes done. Patched: rc 0, 1100 includes. strace on a 2-level nested include file, included twice: every include fd is closed exactly once (fds 4/5 are reused), and the main file is closed once by yy_load_next_buf. No double close. Golden: 44 pass, 2 informational notes ("line order differs (reordered)" on cir/subckt_tests and opt/vci). I checked those: the sorted stdout is identical, and only the pointer-ordered OP printout lines move, as the finding says. a4-include-in-subckt.cir gives the same (pre-existing) syntax error before and after.
