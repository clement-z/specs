# B-154: JSON export writes the readers count as "writers"
severity: med | category: bug | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
## Related
B-163 (other JSON defects), B-164 (electrical nets always 0/0).

- Discovery seeds: S3.12 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-13.
## Verification
Patched the scratch build and ran the golden check: exactly these 8 values differ, nothing else.
