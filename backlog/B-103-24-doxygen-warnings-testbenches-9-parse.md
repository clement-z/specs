# B-103: 24 Doxygen warnings from testbenches and 9 from parse_element.cpp because macros are not expanded
severity: low | category: docs | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: doc/Doxyfile (PREDEFINED, EXCLUDE), src/tb/*_tb.cpp, src/parser/parse_element.cpp:65,146,195,278,374,471,576,629,697
## Problem
`SC_MODULE(...)` testbenches and the `DECLARE_{UNI,BI}DIR_ELEMENT` macro classes are invisible to Doxygen (`PREDEFINED` is empty, `MACRO_EXPANSION` not enabled), giving "documented symbol 'X::run_1' was not declared or defined" / "no matching class member found" for 12 testbench files (24 warnings) and "no uniquely matching class member" for 9 `XxxElement::create` definitions. Also 16 "parameters of ... not documented" for waveguide/mzi/crow/octane constructors.
## Evidence
`doc/Doxyfile` diff against `doxygen -g`: `PREDEFINED =` empty; warnings listed in `/tmp/ws-review-A6/doxy2/warnings.txt` (77 lines in total, 61 non-config).
## Expected
A clean doxygen run (or `WARN_AS_ERROR` possible in CI).
## Suggested fix
Choose between excluding `src/tb` from `INPUT` (`EXCLUDE = src/tb`) and enabling `MACRO_EXPANSION = YES` + `EXPAND_ONLY_PREDEF` with `PREDEFINED = SC_MODULE(x)="class x : public sc_module"`; same for the element macros. Document the 16 ctor params or set `WARN_NO_PARAMDOC = NO` (already) and live with them.
## Golden impact
none.
## Related
B-083, B-102.

- Discovery seeds: S4.22 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-28.
## Verification
Ran doxygen 1.9.1 on the unchanged Doxyfile; counted warnings per category.
