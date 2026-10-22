# B-171: The "reentrant" scanner and parser use global/static state
severity: low | category: style/maintainability | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: src/parser/parser.l:58, :101, :189, :191-192, src/parser/parser.y:37, :165-169, src/parser/parse_tree.h:583
## Problem
Global/static state:
- the global `netlist_content_fifo`;
- the static `last_file` in `yy_load_next_buf`;
- the global `cur_pt`, which every grammar action uses instead of the `pt` parse-param;
- the static `internal_subckt_count`/`delete_me_npos`;
- the static `ParseTree::global_assignments`.

`cur_pt` is left dangling after each flatten sub-parse (it points to a destroyed stack `sub_pt`). `last_file` is not
reset after `fclose`, a latent double-close if the last queued buffer were ever a file.

This blocks parsing from a library (the parser sources are also excluded from libspecs) or in parallel.
## Evidence
Code at the cited lines.
## Expected
All state lives in `ParserState` (scanner extra) and in the `pt` parameter.
## Suggested fix
1. Mechanical and behaviour-neutral: replace `cur_pt` with `pt` in parser.y actions.
2. Move the fifo, `last_file` and the counters into `ParserState`.
3. Set `last_file = nullptr` after `fclose`.
## Golden impact
None.
## Related
B-170, B-172.

- Discovery seeds: S3.22 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-34.
## Verification
Code reading.
