# B-146: Flatten merges subckt port nets against the pre-flatten map (E115, lost type check)
severity: high | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_tree.cpp:899, src/parser/parse_tree.cpp:902, src/parser/parser.y:513-541
## Problem
`flatten()` looks up the external net in `nets` (the parent map before flattening), not in `nets_flat`. X-element
nets are never registered in the parent (`element.x.with_args` uses `net_name_base`, not `net_name`). So when two
instances share a net that no plain element uses, the second instance overwrites the first one's reader/writer
counts and no type check runs. A net with two subckt drivers stays unidirectional (E115). A net that is optical in
one instance and electrical in another is accepted, and elaboration then fails with a cryptic message.
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
## Golden impact
None. Verified: `ROOT/out` in subckt_tests (an X-to-X net) changes from (w0,r1) to (w1,r1), which is still
unidirectional, and its JSON `readers` is unchanged.
## Related
B-152 (unused port crash in the same loop), B-155 (the "Wrong signal type for 0" message).

- Discovery seeds: S3.01 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-03.
## Verification
Patched a scratch copy. `a4-x-shared-net` then runs (rc 0) and its OP values are identical to the flat netlist
(`mid:0/mid:1 = 1 mW`). `a4-x-net-type-mismatch` gives `Could not reconcile nets: ROOT/X2/e and ROOT/n`. Golden
cir/cir-json/opt are unchanged. The change only fixes the lookup map, so it is safe. Currently affected circuits
either crash (E115), fail with a wrong message, or carry wrong counts.
