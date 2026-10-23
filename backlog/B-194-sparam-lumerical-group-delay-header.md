# B-194: Lumerical `.sparam` blocks with a group-delay header field are rejected
severity: low | category: parser | effort: S | fix-class: needs-design | verdict: CONFIRMED | status: open
location: src/io/sparam_file.cpp (Lumerical/SiEPIC block header parsing)

## Problem
Recorded by the second SPAR discussion (`backlog/sprint-2026-10c/SPAR-discussion-2.md`). INTERCONNECT's
`.sparam` format has an "option 3" block header with a 7th field: the group delay of the entry at the centre
frequency f_c = (f_1 + f_N)/2; the stored phase then has 2 pi tau_g (f - f_c) removed. SPAR rejects such files
("not supported", PF03 report, decision 8).

## Expected
Such files load, with the phase restored: phase + 2 pi tau_g (f - f_c) (in the file's convention, before the
conversion to SPECS's).

## Suggested fix
Accept the 7th field; add the linear phase back per entry; a fixture written in that form must give the same
S as the plain form (phase-convention test pattern of `tests/sparam/`).

## Golden impact
None for existing cases; new fixture and test.
