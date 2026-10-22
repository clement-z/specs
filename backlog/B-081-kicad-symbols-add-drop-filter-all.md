# B-081: KiCad symbols ADD_DROP_FILTER, ALL_PASS_FILTER and PCM produce netlist lines SPECS cannot parse
severity: med | category: docs | effort: M | fix-class: needs-design | verdict: CONFIRMED
location: utils/specs.kicad_sym:2-69 (ADD_DROP_FILTER, Ref `ADROP`), :70-120 (ALL_PASS_FILTER, Ref `APASS`), :1467-1513 (PCM, Ref `PCM`, Sim.Params `levels=63` at :1480)
## Problem
The lexer has no prefix for `ADROP`, `APASS` or `PCM` (`src/parser/parser.l:362-457` lists X, WG, MERGER, SPLITTER, COUPLER, PSHIFT, MZI, CROSSING, CWSRC, VLSRC, EVLSRC, PROBE, MLPROBE, PDET, PCMCELL, PWR). Placing any of the three in KiCad gives a netlist line that fails with a syntax error at the reference. The PCM template also uses `levels=63`, which is not a PCMCELL keyword (accepted: `MELT_ENERGY/EMELT/E_MELT`, `N/NSTATES/NLEVELS/N_STATES/N_LEVELS`, `K/INITIAL_STATE`, `SP/TC/T_C`, `EP/TA/T_A`, `TANH_COEF`; parse_element.cpp:1225-1235). The online manual's KiCad tutorial advertises the library as is.
## Evidence
`backlog/repro/a6-kicad-templates.sh`:
```
adrop1 a b c d neff=1 ng=1 k1=0.5 k2=0.5 radius=10e-6 att=0  -> Parsing error ... offending token: "adrop1"
apass1 a b neff=1 ng=1 k=0.5 radius=10e-6 att=0              -> Parsing error ... offending token: "apass1"
pcm1 a b levels=63                                           -> Parsing error ... offending token: "pcm1"
pcmcell1 a b levels=63                                       -> Unknown keyword: levels
```
## Expected
Every symbol in the library maps to an existing element and valid kwargs.
## Suggested fix
Choices: (a) delete the ADD_DROP_FILTER/ALL_PASS_FILTER symbols (no such element exists; build them from COUPLER+WG, as `circuits/add_drop.cir` does) or implement the elements; (b) re-reference PCM as `PCMCELL` with a valid Sim.Params (e.g. `N=64 EMELT=...`; the physical default values need the device owner, see F-A3 PCMCELL findings). Also missing: symbols for PWR (power meter) and `X` subcircuit instances.
## Golden impact
none.
## Related
B-082, B-080.

- Discovery seeds: S3.18 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-06.
## Verification
Netlist lines equivalent to each template run through `specs -n`; lexer rule list read from parser.l. Rejected part of seed S3.18: the claim that KiCad's default net names `Net-(...)` break SPECS is not true for the SPICE exporter, see Seed dispositions.
