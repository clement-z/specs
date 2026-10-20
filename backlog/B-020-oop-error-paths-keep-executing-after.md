# B-020: OOP error paths keep executing after `sc_stop()`; queue reset at thread start
severity: low | category: memory/UB | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/optical_output_port.cpp:97-98,107-113,119,131-139
## Problem
(a) On an empty queue the thread prints an error, calls `sc_stop()` (which does not return control) and falls
through to `m_queue.top()` on an empty vector: UB. (b) On a time desync it calls `sc_stop()` and still writes
the wrong-time value. (c) `m_queue = queue_type();` at thread start drops anything enqueued earlier in the
initialization phase, while its `sc_event_queue` notification stays: that leads to path (a). None of these paths
is reachable from today's CLI netlists (all sources wait for `enable`), so this is latent.
## Evidence
Code reading: `if (true) sc_stop(); else continue;` (lines 111-112, 137-138), followed by `m_queue.top()`
(line 119) and `m_port->write` (line 185). The queue is a default-constructed member, so line 98 is redundant
except for the drop.
## Expected
After a fatal inconsistency, stop without UB and without emitting.
## Suggested fix
```diff
-    // Initialize output queue
-    m_queue = queue_type();
-
@@ empty-queue branch
-            if (true) sc_stop();
-            else      continue;
+            sc_stop();
+            continue;
@@ desync branch
-            if (true) sc_stop();
-            else      continue;
+            sc_stop();
+            continue;
```
(Lines 215-216 in the `#if 0` block can be left alone; they are removed by B-027.)
## Golden impact
None (validated: 44/44 golden pass with exactly these changes plus B-006/08/17/18).
## Related
B-027. These errors (like other `sc_stop()` error paths, e.g. GTD) still exit with status 0.

- Discovery seeds: S1.06, S1.07 (see `backlog/README.md` for seed numbering).
- Review ids: F-A1-20.
## Verification
Code reading and a golden run of the patched binary. Safe because the removed reset only drops entries that the
`sc_event_queue` still expects, and the two `continue`s only affect paths that are already errors.
