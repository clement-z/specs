# B-144: Uni writer connected first to a bidirectional net steals the bidir device's slot
severity: high | category: bug | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.h:828-831, src/parser/parse_tree.h:881-882
## Problem
On a bidirectional net, `connect_uni()` gives every unidirectional writer slot `max(m_connect_writer_count, 1)`,
i.e. always signal `:1`. `connect_bi()` gives the bidir device slot `m_connect_writer_count`. If the uni writer
(source, splitter, crossing, ...) is elaborated before the bidir device, the writer count is already 1, so the bidir
device also writes `:1` and reads `:0`, which nobody writes. Light from the uni writer is silently lost. The result
depends on the netlist line order (elaboration order). Two uni writers on one bidir net both get slot 1, which gives
SystemC E115.
## Evidence
`parse_tree.h:828-831`:
```
int i_write = pt->nets.at(net_name).m_connect_writer_count;
i_write = max(i_write, 1);// If there are already 2 writers, act as a reader for the second one ...
int i_read = (i_write + 1) % 2;
int i = as_writer ? i_write : i_read;
```
The two netlists differ only in line order (`cwsrc1 a` drives the output port of `wg1 b a`, so `a` has 2 writers):
- `backlog/repro/a4-uni-writer-first.cir` (source first): rc 0, `ROOT/b:0 = 0.000000 W`: nothing reaches b.
- `backlog/repro/a4-uni-writer-second.cir` (wg1 first): rc 0, `ROOT/b:0 = 0.001000 W`.
- A2's `backlog/repro/a2-bidir-source-first.cir` (`cwsrc1 in` + `wg1 in& out`): `in:1 = 1 W`, `out:0 = out:1 = 0 W`.
- `backlog/repro/a4-two-sources-one-net.cir` (2 CW sources + probe on one net): rc 1, `E115 ... signal ROOT/a:1`.
## Expected
Every writer on a bidir net gets its own slot. The bidir device reads the slot written by the other side, whatever the
creation order.
## Suggested fix
Candidate (tested in a scratch build): writers take the next free slot, and readers keep their current rule.
```
-        int i_write = pt->nets.at(net_name).m_connect_writer_count;
-        i_write = max(i_write, 1);// ...
-        int i_read = (i_write + 1) % 2;
-        int i = as_writer ? i_write : i_read;
+        int n_writers = pt->nets.at(net_name).m_connect_writer_count;
+        // a writer takes the next free slot; a reader reads the slot of the first writer
+        int i_read = (max(n_writers, 1) + 1) % 2;
+        int i = as_writer ? n_writers : i_read;
```
With it, `a4-uni-writer-first` gives `b:0 = 1 mW` (same as `-second`), A2's repro gives `out:0 = 1 W`, and
`a4-two-sources-one-net` runs (rc 0). It is filed as needs-design because the slot convention
(`:0`/`:1`, who reads what) has to be stated once for `connect_uni`, `connect_bi` and `upgrade_signal` (B-145),
and because it changes the results of these circuits.
## Golden impact
None. I checked the candidate on the cir/cir-json/opt cases and it changed nothing: no golden circuit has a
bidirectional net.
## Related
B-145 (same slot convention), B-176 (elaboration order depends on pointer order), A2's bidir findings.

- Review ids: F-A4-01.
## Verification
Ran the 4 repros on `/tmp/ws-review-build/specs` and on the patched scratch build. Ran the golden check
(cir|cir-json|opt) on the patched build: only the B-154 JSON diffs remained, and they come from another patch in
the same tree.
