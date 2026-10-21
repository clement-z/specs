# B-146: Flatten merges subckt port nets against the pre-flatten map (E115, lost type check)
severity: high | category: bug | effort: XS | fix-class: needs-human | verdict: CONFIRMED | status: fixed in 5912b6a (sprint 2026-10c phase A; cherry-pick of P2-ready/b146 8129020)
location: src/parser/parse_tree.cpp:899, src/parser/parse_tree.cpp:902, src/parser/parser.y:513-541
## Problem
`flatten()` looks up the external net in `nets` (the parent map before flattening), not in `nets_flat`. X-element
nets are never registered in the parent (`element.x.with_args` uses `net_name_base`, not `net_name`). So when two
instances share a net that no plain element uses, the second instance overwrites the first one's reader/writer
counts and no type check runs. A net with two subckt drivers stays unidirectional (E115). A net that is optical in
one instance and electrical in another is accepted, and elaboration then fails with a cryptic message.

Refinement from the second check: a net with two subckt drivers stays unidirectional. It gives E115 when both write in the same delta, and otherwise **runs silently with last-write-wins** (`backlog/repro/v7-x-shared-delayed.cir`, flat equivalent `v7-x-shared-delayed-flat.cir`).
## Evidence
- `backlog/repro/a4-x-shared-net.cir` (`x1 in1 mid s`, `x2 in2 mid s`): rc 1,
  `Error: (E115) sc_signal<T> cannot have more than one driver: signal ROOT/mid ... ROOT/X2/WG1 ... ROOT/X1/WG1`.
  The flat equivalent `backlog/repro/a4-x-shared-net-flat.cir`: rc 0, `mid` bidirectional.
- `backlog/repro/a4-x-net-type-mismatch.cir`: rc 1, `Wrong signal type for 0 connected to ROOT/n`. The flat version
  gives the proper parse error `net already exists with a different type`.
## Expected
Same behaviour as the flat netlist.
## Suggested fix
`parse_tree.cpp:899,902`:
```
-            if (nets.find(external_net_name) != nets.end())
+            if (nets_flat.find(external_net_name) != nets_flat.end())
-                bool ok = sub_pt.nets[internal_net_name].combine_with(nets.at(external_net_name));
+                bool ok = sub_pt.nets[internal_net_name].combine_with(nets_flat.at(external_net_name));
```

Classified `needs-human` only because the fix changes the results of netlists that run today (with wrong results). The change itself is mechanical; a maintainer sign-off is enough.
## Golden impact
None for the golden cases (`--json` and stdout of `subckt_tests`, `add_drop`, `add_drop_sub` are byte-identical). It changes results wherever two or more subckt instances share a port net: reader/writer counts and the `&` flag now accumulate as in the flat netlist. For example, `v7-x-shared-delayed.cir` runs today (rc 0, mid = 2 mW last-write-wins) and after the fix matches its flat equivalent (mid:0 = 1 mW, mid:1 = 2 mW).
## Related
B-152 (unused port crash in the same loop), B-155 (the "Wrong signal type for 0" message).

- Discovery seeds: S3.01 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-03.
## Verification
Patched a scratch copy. `a4-x-shared-net` then runs (rc 0) and its OP values are identical to the flat netlist
(`mid:0/mid:1 = 1 mW`). `a4-x-net-type-mismatch` gives `Could not reconcile nets: ROOT/X2/e and ROOT/n`. Golden
cir/cir-json/opt are unchanged. The change only fixes the lookup map, so it is safe. Currently affected circuits
either crash (E115), fail with a wrong message, or carry wrong counts.

**Independent second check (V7, of F-A4-03): ADJUST.** Method: applied the 2-token fix, rebuilt; ran the repros, subckt_tests/add_drop/add_drop_sub (JSON and stdout), a delayed two-instance case; golden check 44/44

The fix works as claimed. `a4-x-shared-net` goes from rc 1 (E115) to rc 0, with mid:0 and mid:1 = 1 mW, identical to the flat netlist. `a4-x-net-type-mismatch` now gives `Could not reconcile nets: ROOT/X2/e and ROOT/n`. The `--json` output of subckt_tests, add_drop and add_drop_sub is byte-identical before and after (`to_json` prints `readers` for both counts), and stdout is identical.
**However, it changes the results of netlists that run today (rc 0).** In `backlog/repro/v7-x-shared-delayed.cir`, two instances of `wg1 a b` drive `mid`, and a 1 cm delay waveguide puts the two writes at different times, so there is no E115. Base: rc 0, `mid` is unidirectional with 2 drivers and the last write wins (mid = 2 mW), and in1/in2 are unidirectional. With the fix, mid becomes bidirectional (mid:0 = 1 mW, mid:1 = 2 mW, and light propagates back on in1 and in2). That is identical to the flat netlist `v7-x-shared-delayed-flat.cir`.
Changes needed:
- fix-class: easy-unattended -> needs-human. The new results are the correct, flat-equivalent ones, but valid running netlists change.
- Problem text: "a net with two subckt drivers stays unidirectional: E115 if both write in the same delta, otherwise it runs silently with last-write-wins".
- Golden impact: "None for golden cases. It changes results only where ≥2 subckt instances share a port net: writer and reader counts and the `&` flag now accumulate as in the flat netlist."
The fix itself is unique and local, and the location lines are correct.

**Orchestrator:** Reclassified easy-unattended -> needs-human: the fix (correctly) changes the results of netlists that run today.

## Sprint 2026-10b P2 (needs-human)
The approved fix (both lookups use `nets_flat`) works: `a4-x-shared-net` and `v7-x-shared-delayed` give the same
OP listing and VCD as their flat netlists, and `a4-x-net-type-mismatch` gives "Could not reconcile nets: ROOT/X2/e
and ROOT/n". It was **not landed** on `ws/engine` because it changes an existing golden case that the P2 table
predicted unchanged: `cir-json/subckt_tests`, `flat.json` `writers` of `ROOT/out` and `ROOT/X2/N00001` goes from 0
to 1. Each of these nets is written by one subckt instance and read by another, so 1 is the flat-equivalent count
(the second instance used to overwrite the first one's counts). The stdout and VCD of `cir/subckt_tests` are
unchanged. The "byte-identical JSON" of the second check predates B-154 (0dcfda3), when the export printed the
readers count in both fields.

Ready to land: branch `P2-ready/b146` (8129020, one commit on top of `ws/engine` 4083a49): fix, 5 golden cases
(`regress/b146-x-shared-{net,delayed}(-flat)`, `err/b146-x-net-type-mismatch`), the `cir-json/subckt_tests`
re-record and a CHANGELOG line; golden check 120/120 at `--atol 0`. Sign-off needed: accept the JSON count change.

## Sprint 2026-10c phase A (fixed)
Maintainer decision (2026-10-21, follow-up round): land `P2-ready/b146` as is, accepting the JSON `writers` change.
Cherry-picked as 5912b6a, code unchanged. The 5 golden cases were re-recorded with the current build (their branch
baselines predated the B-048 OP sampling and the F-01 name ordering and manifests) and lost their
`unordered_streams` (ordering is name-based since F-01). The only existing-case change is the predicted one:
`cir-json/subckt_tests` `flat.json`, `writers` of `ROOT/out` and `ROOT/X2/N00001` 0 → 1. The hierarchical cases'
OP listings and VCD value sections are identical to their flat references.
