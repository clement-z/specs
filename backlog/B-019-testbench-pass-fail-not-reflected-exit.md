# B-019: Testbench pass/fail is not reflected in the exit status
severity: low | category: test-gap | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/main.cpp:46-63, src/tb/alltestbenches.h (`typedef void (*tb_func_t)()`)
## Problem
`do_test` always returns 0. Testbenches print "Failure!" but return `void`, so `specs -t` cannot fail CI.
## Evidence
`specs -t detector` → one "Failure!" line, rc=0.

Additional evidence (independent report F-A5-04, A5 reviewer):

```
$ specs -t detector > /dev/null; echo $?        -> 0       (stdout contains "Failure!")
$ specs -t mesh > /dev/null; echo $?             -> 139     (the only non-zero outcomes are crashes)
tb results today: wg 3/3, merger 4/4, dc 3/3, splitter 3/3, pcm 3/3, ps 3/3, detector: Failure! (no summary), mzi/ring/lambda/ac_add_drop/ac_crow/mesh: no checks at all
```
## Expected
A failed self-check gives rc≠0.
## Suggested fix
Change `tb_func_t` to return `int` (or count failures in a global the tb sets) and return it from `do_test`. This touches every tb.
## Golden impact
`tb/det` and `tb/detector` exit codes (if they still fail).
## Related
A5 (testing).

- Discovery seeds: S4.07 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A5-04 (A5).
- Review ids: F-A1-19, F-A5-04.
## Verification
Ran it.
