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
