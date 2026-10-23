# Reproducible runs

Two runs of the same netlist with the same SPECS build and the same command line give the same stdout and the
same trace (VCD), apart from the VCD's `$date` header and the `Total runtime` line of the testbenches. This
includes noisy photodetectors: their random numbers come from a seed, and the default seed is fixed. Each run
also writes a manifest next to its trace that records what produced it (feature F-01, sprint 2026-10b).

## Seed

Every random draw comes from one master seed:

| Where | Syntax | Notes |
| --- | --- | --- |
| command line | `specs --seed N -f circuit.cir` | N: decimal integer, 0 to 2^64 − 1. Also applies to `-t` testbenches. Wins over `.options seed` |
| netlist | `.options seed=N` | N: integer. An integer literal (`3000000000`) is exact from 0 to 9223372036854775807 (2^63 − 1; a larger literal is a parse error); in exponent form (`3e9`) N is a double, 0 to 2^53 |
| default | — | **0** |

Each device draws from its own stream, derived from the master seed and the device's hierarchical instance name
(netlist names are upper-cased, e.g. `ROOT/X1/PDET1`):
- adding, removing or reordering other elements does not change a device's random numbers;
- renaming the device, or moving it into another subcircuit instance, does;
- two devices never share a stream.

The streams use `std::mt19937_64` seeded through `std::seed_seq` and SPECS's own uniform and normal
conversions. All three are fully specified, so the numbers are the same with libstdc++ and libc++; across
platforms they are bit-identical as long as `log()` and `sqrt()` are.

What draws random numbers today:
- PDET noise, with `nb=0` (`noise_bypass=false`);
- the PDET dark-field injection: when the total field on a detector is exactly 0, a tiny random field
  (1e-20 √W times a standard normal draw, i.e. about 1e-40 A) is used instead. It is on by default and can be
  turned off per detector with `darkfield=0`; see [netlist.md](netlist.md#pdet);
- the random functions of netlist expressions (`gauss`, `uniform`, `lognormal`, `choice`, F-02), see below.

## Random parameters and Monte-Carlo runs

A random function draws when its line is read, from its own stream of the master seed, named after where it is:
`<instance path>/<owner>/<name>#<n>`, e.g. `ROOT/.param/L#0` for `.param L=gauss(...)` at the top level,
`ROOT/X3/.param/L#0` for the same line in the body of instance X3, `ROOT/WG1/length#0` for
`wg1 a b length=gauss(...)`; `n` counts the draws of the same name before it in the same body. So:
- adding, removing or reordering other lines does not change a draw;
- each instance of a subcircuit draws its own values (its body is read once per instance), unless the function
  has `shared=1` (stream `SUBCKT:<name>/...`: one value for every instance of the subcircuit) or
  `shared="tag"` (stream `SHARED/tag`: one value for every use of the tag).

The seed must be known when a line draws: `--seed N`, or an `.options seed=N` line before the first random
function. An `.options seed` that would change the seed after a draw stops the run with an error naming both
lines (it would not apply to that draw).

`.mc N` (scout F-02, `doc/netlist.md`): Monte-Carlo run k simulates with the seed `s_k`, the first 64-bit output
of the stream `.mc/k` of the master seed (`.mc seed=S`, `--seed` or `.options seed`). Every draw of the run
(random functions and PDET noise) comes from `s_k`, so a run is reproducible on its own: `specs --run K` gives the
same values as run K of the plan, and so does `specs --seed s_k` with the stepped values as `--param` on the
netlist without its `.step`/`.mc` lines. `s_k` does not depend on the `.step` values: every step point sees the
same Monte-Carlo draws. Without `.mc`, every run of a `.step` plan uses the master seed. The seed of each run
is in the run table (`runs.csv`), with every value drawn (`draw_*.npy`) and the table manifest
`table.manifest.json` (version, master seed, command line, netlist and data file hashes, plan). A plan resumed
against a table whose manifest differs (e.g. a changed netlist or data file) stops with an error.

## Order of outputs

Listings and traces follow names, not memory addresses:
- the operating-point listing, the `--vci` dump, the automatic `PROBE{<net>}` probes and the VCD declarations
  are sorted by hierarchical name, in byte order (`strcmp`: upper case before lower case, `ROOT/wg10` before
  `ROOT/wg2`);
- elements are created in netlist order (the "Creating ..." lines of the build step);
- devices are initialised (`init()`), and enabled at the start of an analysis, in name order.

Before sprint 2026-10b these orders followed heap addresses and could change with the compiler, the build type,
a sanitizer or an unrelated code change (B-016, B-176).

## Run manifest

Every run that writes the default trace `<trace>.vcd` (`-f` netlists, `-o <trace>`, the `-t` testbenches) also
writes `<trace>.manifest.json` next to it, before the simulation starts. With the default trace name that is
`traces/delete_me.manifest.json`. `specs --print-manifest` also prints it on stdout, once, at the same point (with
`-n`, right after the circuit is built; nothing is written then and `"trace_file"` is `null`). Example
(`specs -o traces/m -f circuit.cir`):

```json
{
  "format": "specs-run-manifest",
  "format_version": 2,
  "specs_version": "v0.1-215.g4e3e87d",
  "systemc_version": "3.0.2-Accellera",
  "seed": 0,
  "command_line": ["specs", "-o", "traces/m", "-f", "circuit.cir"],
  "netlist_files": [
    {"path": "circuit.cir", "sha256": "4ee4ad07d39c258bf8f70f910989f026d179931a90f649e1a557dd30aae607ae"},
    {"path": "sub.cir", "sha256": "4ff4ac4d7351edd5cb44e5b7ca78ba64f46dbde7128b80a19cf563bc92a861fc"}
  ],
  "data_files": [
    {"path": "ring.s4p", "sha256": "0b3c1e1ba54b7f0bfa1ed5f6e0b5d0d2fc5e8d07fda4bd3dbcd5a0d34cb42c4f"}
  ],
  "trace_file": "traces/m.vcd"
}
```

- `specs_version` is the output of `specs --version` (git describe of the build); `systemc_version` is SystemC's
  `sc_release()`.
- `seed` is the master seed actually used (`--seed`, `.options seed` or the default 0).
- `command_line` is `argv` as received.
- `netlist_files` lists every file the parser opened, once, in the order it first opened them: the `-f` files
  and every `.include` (also those inside subcircuit bodies), as the path used to open them (relative to the
  working directory), with the SHA-256 of their content when the manifest was written (`sha256sum` gives the
  same value; `null` if the file could not be read).
- `data_files` lists, in the same form, every data file the devices read while the circuit was built, once, in
  the order they were first opened: the S-parameter files of `SPAR` (`file=`, resolved as `.include`: relative
  to the netlist file first) and the value files of `VLSRC`/`EVLSRC` (relative to the working directory). It is
  `[]` when there is none.
- `format_version` 2 added `data_files`: a version 1 manifest did not record the data files.
- `trace_file` is the VCD SPECS opened. SystemC creates the file at the first traced time step, so a run that
  never advances time (a `.op` alone) has a manifest but no VCD.
- The manifest has no date, host or absolute path: two runs with the same build, inputs and command line give
  the same bytes.

To reproduce a run: check out and build `specs_version`, check the files against `netlist_files` and
`data_files`, and run `command_line` again in the same working directory. If the manifest cannot be written,
SPECS stops with `Error: cannot open run manifest file "..."` (exit 1), before the simulation.
