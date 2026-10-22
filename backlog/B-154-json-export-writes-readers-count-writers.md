# B-154: JSON export writes the readers count as "writers"
severity: med | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 0dcfda3
location: src/parser/parse_tree.cpp:573
## Problem
`ParseNet::to_json` emits `"writers":` followed by `m_readers_count`. `draw_graph_from_json.py` and any `--json`
consumer show wrong writer counts.
## Evidence
`backlog/repro/a4-json.cir` (1 source, 2 probes on `a`) with `specs -n --json out.json`:
`"ROOT/a":{"type":"OANALOG","size":1,"bidirectional":0,"readers":2,"writers":2}`. It should be `"writers":1`.
## Expected
`"writers":<m_writers_count>`.
## Suggested fix
`parse_tree.cpp:573`: `ss << "\"writers\":" << m_writers_count;`
## Golden impact
Verified diffs; the baseline must be re-recorded:
- cir-json/add_drop: `ROOT/add.writers 1→0`, `ROOT/drop.writers 0→1`, `ROOT/out.writers 0→1`.
- cir-json/add_drop_pyspecs: `ROOT/add.writers 1→0`, `ROOT/in.writers 2→1`.
- cir-json/subckt_tests: `ROOT/add.writers 1→0`, `ROOT/drop.writers 0→1`, `ROOT/out_.writers 0→1`.

Correction from the second check: exactly **10** values change, not 8: cir-json/add_drop `ROOT/add 1->0`, `ROOT/drop 0->1`, `ROOT/out 0->1`; cir-json/add_drop_pyspecs `ROOT/add 1->0`, `ROOT/in 2->1`; cir-json/subckt_tests `ROOT/add 1->0`, `ROOT/drop 0->1`, `ROOT/out_ 0->1`, `ROOT/X2/N00001 1->0`, `ROOT/out 1->0`. cir-json/add_drop_sub is unchanged. Re-record the baseline in the same commit.
## Related
B-163 (other JSON defects), B-164 (electrical nets always 0/0).

- Discovery seeds: S3.12 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-13.
## Verification
Patched the scratch build and ran the golden check: exactly these 8 values differ, nothing else.

**Independent second check (V8, of F-A4-13): ADJUST.** Method: applied the one-line fix (m_readers_count -> m_writers_count), rebuilt, golden --atol 0, compared flat.json reader/writer counts of every cir-json case against the baseline; checked determinism (3 runs, identical md5)

Bug confirmed: a4-json.cir `ROOT/a` gives writers 2 -> 1 (readers 2). Fix correct. **The golden impact is incomplete: 10 values change, not 8.** cir-json/add_drop: `ROOT/add 1->0`, `ROOT/drop 0->1`, `ROOT/out 0->1` (as claimed). cir-json/add_drop_pyspecs: `ROOT/add 1->0`, `ROOT/in 2->1` (as claimed). cir-json/subckt_tests: `ROOT/add 1->0`, `ROOT/drop 0->1`, `ROOT/out_ 0->1`, **plus `ROOT/X2/N00001.writers 1->0` and `ROOT/out.writers 1->0`**. cir-json/add_drop_sub: no change. Nothing else in golden changes. Replace the Golden impact and Verification text with these 10 values ("exactly these 10 values differ"). The re-record is still needed.

**Orchestrator:** Golden impact corrected to the 10 changed values.
