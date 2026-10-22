# Legacy netlists (pre-release syntax)

`double_ring.cir`, `triple_ring.cir` and `paul_simple_ring.cir` are written in the netlist language of a
pre-release version of SPECS: `one`, `clk` and `bsrc` sources, unnamed `wg`/`dc`/`pdet` elements (a 4-net `dc`
coupler, a 3-net `pdet`), `*tracefile`/`*tracenet` comments. The current parser does not accept that syntax:
`specs -f circuits/legacy/double_ring.cir` stops with a parse error at line 4 (`one 0`).

They are kept, unchanged, for reference: they are the only examples of cascades of two and three ring
resonators driven by a bitstream, with the ring lengths used at the time. They are not maintained and are not
examples to copy; use the netlists in `circuits/` and the syntax in `doc/netlist.md` instead. Porting them
needs a bitstream source in the current netlist language and the resonance wavelengths and couplings
re-derived (backlog B-133).
