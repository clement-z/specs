# B-145: upgrade_signal swaps signals when the existing port is a reader, so it loses the light
severity: high | category: bug | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parse_tree.cpp:230-237
## Problem
`upgrade_signal()` adds a second signal when a bidirectional device joins a net that was created unidirectional and
already has one bound port. If that port is a reader, the code swaps `[0]` and `[1]`. The reader stays on the
original signal, now at `[1]`, while the next `connect_bi()` (writer count 0) writes `[0]` and reads `[1]`. The
reader never sees the bidirectional device's output, so forward light is silently dropped. The comment says "if a
writer" in the reader branch.
## Evidence
`backlog/repro/a4-upgrade-reader.cir`: `merger1 c a out` is created first (it sits on bidir net `a`), then
`wg1 e& c` upgrades `c`. Output: `ROOT/c = 0 W` (the merger reads this), `ROOT/c:1 = 4 mW` (wg1 writes this), and
`ROOT/out = 0.5 mW`. Reference `backlog/repro/a4-upgrade-reader-ref.cir` (the same circuit without `&`):
`ROOT/c = 4 mW`, `ROOT/out = 1.341 mW`.
## Expected
The reader receives the bidir device's forward output: `out = 1.341 mW`, as in the reference.
## Suggested fix
Remove the `swap(...)` at :234: the reader stays on `[0]`, which the bidir device writes when
`m_connect_writer_count == 0`. Keep `m_writers_count++` and fix the comment. With this candidate (plus B-144),
`a4-upgrade-reader` gives `c = 4 mW`, `out = 1.341 mW`. Needs-design for the same reason as B-144: fix both
together under one documented slot convention.
## Golden impact
None (candidate checked with the golden harness).
## Related
B-144.

- Review ids: F-A4-02.
## Verification
Ran the repro and its reference on both builds. The swap path runs only when `m_connect_count > 0` at upgrade time
(:202), which this topology triggers.

**Independent second check (V7, of F-A4-02): ADJUST.** Method: reproduced; applied "remove the swap" (on top of B-144) and wrote a counter-case; prototyped an alternative fix; golden check 44/44 for both

The problem is real. Base `a4-upgrade-reader` gives c = 0, c:1 = 4 mW and out = 0.5 mW. The expected value is 1.341 mW: (4+1+4cos(4.094))/2 mW, which matches the `-ref` run. With the swap removed it gives c = 4 mW and out = 1.341 mW.
**The suggested fix breaks a case that works today.** The swap exists for the case where the upgrading bidir device *reads* the net and the net's uni writer is created later. `/tmp/ws-review-V7/run/v7-upgrade-reader-case2.cir` is that case: merger1 reads c and is created first, `wg1 c e&` reads c, and `cwsrc3 c` is listed after wg1. Base gives out = 1.341 mW, correct and equal to its no-`&` reference. With the swap removed it gives out = 0.5 mW: the light is lost. This happens with or without B-144.
Replace the suggested fix with: "Make the swap depend on the upgrading device's role on that net. Add `bool device_writes_net` to `upgrade_signal()`. Swap only when the device reads the net (keep today's behaviour). When the device writes the net (WG nets[1]; DC/MZI/crossing nets[2..3]; PS nets[1]), keep the existing reader on [0]. Fix the 'if a writer' comment."
I prototyped this for WG only, together with B-144 (`/tmp/ws-review-V7/f01-alt02.diff`). It gives a4-upgrade-reader out = 1.341 mW and case2 out = 1.341 mW, and the golden check is unchanged. Add case2 as a second repro. The rest stands: needs-design, severity high, location 230-237 with the swap at :234.
