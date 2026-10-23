# B-069: Unreachable research devices with latent bugs: OCTANE composites, the GenericTransmissionDevice family, BitstreamSource
severity: low | category: style/maintainability | effort: M | fix-class: needs-human | verdict: CONFIRMED
location: src/devices/octane_cell.*, src/devices/octane_segment.*, src/devices/octane_matrix.*, src/devices/generic_transmission_device.*, src/devices/generic_2x2_coupler.h, src/devices/generic_waveguide.h, src/devices/bitstream_source.*, src/specs.cpp:1,15
## Problem
None of these classes is reachable from a netlist or a testbench. GTD users are commented out in tb/directional_coupler_tb.cpp:110 and tb/wg_tb.cpp:106, and `GenericWaveguide` `exit(1)`s in its constructor. They carry known bugs.
- **OCTANE:**
  - opaque PCMs (B-051);
  - hard-coded physics (λ, neff = ng = 2.2, zero losses);
  - duplicated `port_in_type` typedefs at global scope (octane_segment.h:11-14, octane_matrix.h:10-13, "TODO: remove");
  - children named without hierarchy;
  - the full and compact init paths are copy-pasted.
- **GTD:**
  - `input_on_i_output_on_j` latches S_ij at the first λ and calls `sc_stop()` on any λ change, but keeps computing with mismatched λ (generic_transmission_device.cpp:202-208).
  - It keeps `last_signal` across DC sweep points while the OOPs are `reset()` (specs.cpp:161-163) in `m_use_deltas` mode, so from point 2 on it emits Δ instead of the value.
  - `input_on_i` is unused.
  - A debug print (B-065).
  - A -Wmaybe-uninitialized warning (B-075).
- **BitstreamSource:**
  - `while (i.good()) { i >> value; push_back }` appends a spurious 0 at EOF and never reports an unopenable file (bitstream_source.h:91-96).
  - `int value` vs `unsigned long operand_type`.
  - The saturation test uses `m_current_value` instead of `val` (bitstream_source.cpp:29).
  - "CLK tick" is printed at every clock edge (:53).
  - `using namespace std;` in a header (bitstream_source.h:9).
  - specs.cpp:1 includes it for no reason; specs.cpp:15 includes the GTD header for no reason.
## Evidence
grep for users: none outside their own files and comments.
## Expected
Either maintained and tested (reachable from a netlist or tb), or removed.
## Suggested fix
The maintainer decides per family whether to keep it (OCTANE is funded work, per the paper's acknowledgments) or delete it. If kept, file the listed bugs as separate items. Either way, drop the unneeded includes at specs.cpp:1 and :15.
## Golden impact
None for deletion.
## Related
B-051, B-065, B-068, B-075.

- Discovery seeds: S1.08 (see `backlog/README.md` for seed numbering).
- Review ids: F-A3-23.
## Verification
Code reading and grep.

**2026-10-23 (PD2b):** the GTD family (`GenericTransmissionDevice`, `Generic2x2Coupler`, `GenericWaveguide`) and the delta-write mode of the output ports were removed in 54b15dc (maintainer decision 2026-10-23), with the include at specs.cpp:15; the GTD bullets above are obsolete. OCTANE and BitstreamSource stay open.
