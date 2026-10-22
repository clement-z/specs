# F-12: Netlist compatibility and ergonomics pack

value 4 | effort S-M | risk low (two items change results of valid netlists and are flagged) | prerequisites:
none | horizon 1 (most items)

## Problem / user story
*As a SPICE-literate newcomer I expect `10u`, `.ends NAME`, `.dc WL(cwsrc1)` and the paper's own netlist to
work.* Today: no SI suffixes (`src/parser/parser.l:8` `DOUBLE`, `:340-351`), `.ends NAME` is a syntax error,
`.dc` accepts only `/elem/ATTR` (`src/parser/parser.y:775-806`) so the paper's and the cheatsheet's
`.dc WL(cwsrc1) ...` fail, `.save` has a token but no rule (`parser.l:461`), `.include` resolves relative to the
working directory (`parser.l:243-260`), only the first analysis runs (`src/parser/parse_tree.cpp:808-809`) and
only one `.dc` sweep is allowed (`parser.y:405-409`), the sweep endpoint is excluded
(`src/utils/general_utils.h:30-33`, `src/parser/parse_analysis.h:59-66`), expressions have no functions
(`parser.y` expression section), elaboration-time errors carry no file:line, and 4 of 9 shipped netlists plus
the paper's Fig. 1b do not parse (discovery report 03 §4, report 05 §3). The paper's netlist also uses a `dc1`
coupler prefix that is now `COUPLER`.

## Prior art
ngspice/Xyce syntax (`.step`, `.temp`, SI suffixes with `meg`), KiCad SPICE export conventions, the SPICE habit
of `.ends NAME`; SAX/gdsfactory YAML as the non-SPICE alternative (F-13).

## Proposed design
1. **Lexer:** `{DOUBLE}{SUFFIX}?{UNIT}?` with suffixes `f p n u m k meg g t` (SPICE semantics: `m` = milli,
   `meg` = mega; case-insensitive lexer, so document that `M` is milli) and ignored trailing unit letters
   (`10um`, `3.5nm`, `2dB`). `.ends [NAME]`. `.temp` (F-09). `.save` (F-05).
2. **Grammar:** `.dc WL(cwsrc1) a b c` and `.dc cwsrc1 wl a b c` as aliases of `/cwsrc1/WL`; functions
   `sqrt exp log log10 sin cos tan abs min max pow` plus the random ones (F-02) in `constant`; `DC` accepted as
   an alias prefix for `COUPLER` (paper netlist) or documented as renamed.
3. **Semantics:** `.include` relative to the including file, then the working directory; several analyses per
   netlist run sequentially in forked children (F-02 runner) with suffixed outputs; `.dc` includes the endpoint
   (**changes results**: `add_drop.cir` gives 31 points instead of 30; needs-human and a golden re-record with a
   note); file:line attached to every `ParseElement` so elaboration errors can cite them.
4. **Examples:** port `double_ring.cir`, `triple_ring.cir`, `paul_simple_ring.cir` to current syntax once a
   bitstream source exists in the netlist (F-18), fix `subckt_test.cir`, add the missing CSV or a generator for
   `paul_3ring_puf.cir`, and add the paper's Fig. 1b netlist as `circuits/paper_fig1b.cir`.
5. **CLI:** `--param` (F-02).

## Touch points
`src/parser/parser.l:8, 243-260, 294-312, 340-351, 460-470` · `src/parser/parser.y:185-240, 405-418, 775-806,
expression rules` · `src/parser/parse_tree.cpp:808-809`, `parse_tree.h` (element locations) ·
`src/utils/general_utils.h:30-33`, `src/parser/parse_analysis.h:59-66` · `circuits/*.cir` (`git add -f`) ·
`doc/resources/cheatsheet.pdf` (regenerate from the registry, F-20/F-26).

## Validation plan
- Parser tests through the golden harness's `-n --json` cases: every `circuits/*.cir` parses; new cases for
  suffixes (`10u == 1e-5`), `.ends NAME`, `.dc WL()` alias equality with `/elem/WL`, nested include paths.
- Golden re-record only for the endpoint change, with `--note`.
- The paper netlist runs end to end and matches `add_drop_pyspecs.cir`.

## Effort breakdown
Suffixes 0.5 d · `.ends`/aliases/`DC` prefix 1 d · include path 0.5 d · functions 1 d · endpoint + multi-analysis
1.5 d · locations in errors 1 d · examples 1 d · tests 1 d.

## Open questions for the maintainer
- Endpoint inclusion: adopt SPICE semantics (proposed) and accept the one-time golden change?
- Should `M`/`m` ambiguity be resolved SPICE-style (both milli) or rejected with a hint?
- Keep the three legacy ring netlists (they document the pre-release syntax) or delete them?
