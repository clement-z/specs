# B-007: Default trace path `traces/delete_me` is cwd-relative and rewrites a tracked file
severity: low | category: cli | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: src/specs.cpp:36, src/main.cpp:219-222,368-372
## Problem
Without `-o`, the VCD goes to `./traces/delete_me.vcd`. Outside a directory that has `traces/`, the run parses,
elaborates and starts, then dies with `Error: (E701) cannot open trace file for writing` (rc=1). Inside the repo
root it overwrites the git-tracked `traces/delete_me.vcd`. The help text says "Set the default trace file", but
`.vcd` is always appended: `-o traces/y.vcd` writes `traces/y.vcd.vcd`.
## Evidence
`cd /tmp/x && specs -f st.cir` → E701 after the "SIMULATION" banner. `git ls-files traces` → `traces/delete_me.vcd`.
`backlog/repro/a1-cli.sh` shows `y.vcd.vcd`.

Additional evidence (independent report F-A7-11, A7 reviewer):

```
$ git ls-files traces        -> traces/delete_me.vcd (0 bytes)
scratch repo with the same layout: run `specs -f circuits/add_drop.cir` -> `git status --short`: " M traces/delete_me.vcd"
```
## Expected
A default that works from any cwd (or no VCD unless `-o`), with the error reported before the simulation starts. Accurate help text.
## Suggested fix
Design choice: a default such as `./specs.vcd`, or no trace without `-o`. Create the directory, or check it is
writable at startup. Strip a trailing `.vcd` from `-o`, or document "base name, .vcd appended". Untrack
`traces/delete_me.vcd`.
## Golden impact
Every case that relies on `traces/delete_me.vcd` (all cir/*, most opt/*) if the default changes.
## Related
B-006, B-005.

- Discovery seeds: S1.11 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A7-11 (A7).
- Review ids: F-A1-07, F-A7-11.
## Verification
Ran in an empty dir (E701, rc=1). Ran `-o traces/y.vcd`. Ran `git ls-files`.
