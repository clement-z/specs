#!/usr/bin/env python3
"""crow_gen.py: generate a CROW (coupled-resonator optical waveguide) netlist for SPECS.

Default: the paper's benchmark (ECOC 2023, Fig. 3): N rings, N+1 couplers with power cross-coupling
kp = 0.15, 2N waveguides whose lengths are drawn from N(33.2 um, 5 um) with a seeded RNG (each ring
has two different halves), one source emitting 1 ns pulses at 500 MHz (10 pulses = 20 ns).
Optional WDM: --nwl channels on a 100 GHz grid, all pulsed together.

Other forms:
    --source cw                   a CW source (wavelength --wl) instead of the pulses
    --analysis dc                 a .dc wavelength sweep of the CW source (--dc-start/--dc-stop/--dc-step)
    --preset crow_tb              the circuit of the legacy testbench `specs -t ac_crow`
                                  (src/tb/crow_tb.cpp): identical rings, kp = 0.2, neff 1, ng 2,
                                  4 pulses of 1 ns at 1553 nm, .tran 50 ns; explicit options override it

Usage:
    python3 -I crow_gen.py [N] [--seed 1] [--traceall 0] [--probes 1] [--pulses 10] [--nwl 1]
                           [--kp 0.15] [--lmean 33.2e-6] [--lstd 5e-6] [--tscale -13] > crow.cir

Topology (coupler i: in1 in2 out1 out2; through in1->out1, in2->out2; cross in1->out2, in2->out1):
    coupler 0 : in1 = bus in, out1 = through, in2 = ring1 return, out2 = ring1 forward
    coupler i : in1 = ring i forward, out1 = ring i return, in2 = ring i+1 return, out2 = ring i+1 forward
    coupler N : in2 = add (unused), out2 = drop
    ring i    : WG{i}a (coupler i-1 out2 -> coupler i in1), WG{i}b (coupler i out1 -> coupler i-1 in2)
This is the topology of the CROW device of the testbench (src/devices/crow.cpp).

As a module: `parse_args(argv)` gives the parameters, `ring_lengths(p)` the 2N waveguide lengths exactly
as written in the netlist, `generate(p)` the netlist text (tests/bench/oracles.py uses all three).
"""
import argparse
import random
import sys

C0 = 299792458.0

# The legacy testbench (src/tb/crow_tb.cpp, crow_tb_run): ring length 500 * 1.55 um + 1.55 um / 2,
# split in two equal halves (src/devices/crow.cpp); through power 0.8 per coupler.
PRESETS = {
    "crow_tb": dict(N=3, lmean=(500.0 * 1.55e-6 + 1.55e-6 / 2.0) / 2.0, lstd=0.0, kp=0.2, neff=1.0, ng=2.0,
                    att=2.0, power=1.0, wl=1553e-9, pulses=4, width=1e-9, period=2e-9, duration=50e-9,
                    abstol=1e-8, reltol=1e-4, tscale=-13, traceall=0, probes=1),
}

DEFAULTS = dict(N=3, seed=1, traceall=0, probes=1, pulses=10, nwl=1, kp=0.15, lmean=33.2e-6, lstd=5.0e-6,
                neff=2.2, ng=4.3, att=2.0, power=1e-3, wl=1550e-9, width=1e-9, period=2e-9, tscale=-13,
                abstol=1e-8, reltol=1e-4, duration=None, source="pulses", analysis="tran",
                dc_start=1549e-9, dc_stop=1551e-9, dc_step=0.05e-9)


def build_parser():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("N", type=int, nargs="?", help="number of rings (default 3)")
    ap.add_argument("--preset", choices=sorted(PRESETS), help="start from a named circuit")
    ap.add_argument("--seed", type=int)
    ap.add_argument("--traceall", type=int, help=".options traceall (auto-probe every net)")
    ap.add_argument("--probes", type=int, help="add PROBE on through and drop")
    ap.add_argument("--source", choices=["pulses", "cw"])
    ap.add_argument("--analysis", choices=["tran", "dc"], help="dc: wavelength sweep of the CW source")
    ap.add_argument("--pulses", type=int)
    ap.add_argument("--width", type=float, help="pulse width, s")
    ap.add_argument("--period", type=float, help="pulse period, s")
    ap.add_argument("--nwl", type=int, help="WDM channels on a 100 GHz grid around --wl")
    ap.add_argument("--wl", type=float, help="source wavelength (centre of the WDM grid), m")
    ap.add_argument("--kp", type=float, help="power cross-coupling of every coupler")
    ap.add_argument("--lmean", type=float, help="mean waveguide (half-ring) length, m")
    ap.add_argument("--lstd", type=float, help="standard deviation of the lengths, m (0: identical rings)")
    ap.add_argument("--neff", type=float)
    ap.add_argument("--ng", type=float)
    ap.add_argument("--att", type=float, help="dB/cm")
    ap.add_argument("--power", type=float, help="W")
    ap.add_argument("--tscale", type=int, help="engine timescale, log10(s)")
    ap.add_argument("--abstol", type=float)
    ap.add_argument("--reltol", type=float)
    ap.add_argument("--duration", type=float, help=".tran length, s (default: pulses * period)")
    ap.add_argument("--dc-start", type=float, help="m")
    ap.add_argument("--dc-stop", type=float, help="m")
    ap.add_argument("--dc-step", type=float, help="m")
    return ap


def parse_args(argv=None):
    """Parameters as a namespace: defaults, then the preset, then the explicit options."""
    a = build_parser().parse_args(argv)
    p = dict(DEFAULTS)
    if a.preset:
        p.update(PRESETS[a.preset])
    for k, v in vars(a).items():
        if k != "preset" and v is not None:
            p[k] = v
    if p["analysis"] == "dc":
        p["source"] = "cw"
    if p["N"] < 1:
        sys.exit("crow_gen.py: N must be >= 1")
    return argparse.Namespace(preset=a.preset, **p)


def ring_lengths(p):
    """The 2N lengths [(L_ia, L_ib) for i = 1..N] in m, rounded as written in the netlist."""
    rng = random.Random(p.seed)
    out = []
    for _ in range(p.N):
        la = max(1e-6, rng.gauss(p.lmean, p.lstd))
        lb = max(1e-6, rng.gauss(p.lmean, p.lstd))
        out.append((float("%.9g" % la), float("%.9g" % lb)))
    return out


def wavelengths(p):
    f0 = C0 / p.wl
    return [C0 / (f0 + (i - (p.nwl - 1) / 2) * 100e9) for i in range(p.nwl)]


def generate(p):
    N = p.N
    out = []
    w = out.append
    what = "preset %s" % p.preset if p.preset else "paper-style (ECOC 2023 Fig. 3)"
    w("* CROW with %d rings, %s; seed=%d nwl=%d source=%s analysis=%s"
      % (N, what, p.seed, p.nwl, p.source, p.analysis))
    if p.source == "cw":
        w("CWSRC1 in wl=%.12g power=%g" % (p.wl, p.power))
    else:
        vals = []
        wls = wavelengths(p)
        for k in range(p.pulses):
            t_on = k * p.period
            t_off = t_on + p.width
            for wl in wls:
                vals.append("[%.12g,%.6g,%.12g]" % (t_on, p.power, wl))
            for wl in wls:
                vals.append("[%.12g,0,%.12g]" % (t_off, wl))
        w("VLSRC1 in values=[" + ",".join(vals) + "]")
    for i in range(N + 1):
        in1 = "in" if i == 0 else "r%da_out" % i
        in2 = "add" if i == N else "r%db_out" % (i + 1)
        out1 = "through" if i == 0 else "r%db_in" % i
        out2 = "drop" if i == N else "r%da_in" % (i + 1)
        w("COUPLER%d %s %s %s %s kp=%g" % (i, in1, in2, out1, out2, p.kp))
    for i, (la, lb) in enumerate(ring_lengths(p), start=1):
        w("WG%da r%da_in r%da_out L=%.9g att=%g neff=%g ng=%g" % (i, i, i, la, p.att, p.neff, p.ng))
        w("WG%db r%db_in r%db_out L=%.9g att=%g neff=%g ng=%g" % (i, i, i, lb, p.att, p.neff, p.ng))
    if p.probes:
        w("PROBE1 through")
        w("PROBE2 drop")
    w(".options abstol=%g reltol=%g timescale=%d traceall=%d" % (p.abstol, p.reltol, p.tscale, p.traceall))
    if p.analysis == "dc":
        w(".dc /cwsrc1/WL %.12g %.12g %.12g" % (p.dc_start, p.dc_stop, p.dc_step))
    else:
        dur = p.duration if p.duration else p.pulses * p.period
        w(".tran %g" % dur)
    return "\n".join(out) + "\n"


def main():
    sys.stdout.write(generate(parse_args()))


if __name__ == "__main__":
    main()
