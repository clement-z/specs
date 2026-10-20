#!/usr/bin/env python3
"""crow_gen.py: generate a paper-style CROW netlist for SPECS (ECOC 2023, Fig. 3).

N rings, N+1 unidirectional couplers (kp = power cross-coupling, 0.15 in the paper),
2N waveguides with L_i ~ N(33.2 um, 5.0 um) drawn from a seeded RNG, one source emitting
1 ns pulses at 500 MHz (10 pulses = 20 ns). Optional WDM: --nwl channels on a 100 GHz grid,
all pulsed together.

Usage:
    python3 -I crow_gen.py N [--seed 1] [--traceall 0] [--probes 1] [--pulses 10] [--nwl 1]
                             [--kp 0.15] [--lmean 33.2e-6] [--lstd 5e-6] [--tscale -13] > crow.cir

Topology (coupler i: in1 in2 out1 out2; through in1->out1, in2->out2; cross in1->out2, in2->out1):
    coupler 0 : in1 = bus in, out1 = through, in2 = ring1 return, out2 = ring1 forward
    coupler i : in1 = ring i forward, out1 = ring i return, in2 = ring i+1 return, out2 = ring i+1 forward
    coupler N : in2 = add (unused), out2 = drop
    ring i    : WG{i}a (coupler i-1 out2 -> coupler i in1), WG{i}b (coupler i out1 -> coupler i-1 in2)
"""
import argparse
import random


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("N", type=int, help="number of rings")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--traceall", type=int, default=0, help=".options traceall (auto-probe every net)")
    ap.add_argument("--probes", type=int, default=1, help="add PROBE on through and drop")
    ap.add_argument("--pulses", type=int, default=10)
    ap.add_argument("--nwl", type=int, default=1, help="WDM channels on a 100 GHz grid around 1550 nm")
    ap.add_argument("--kp", type=float, default=0.15, help="power cross-coupling")
    ap.add_argument("--lmean", type=float, default=33.2e-6)
    ap.add_argument("--lstd", type=float, default=5.0e-6)
    ap.add_argument("--neff", type=float, default=2.2)
    ap.add_argument("--ng", type=float, default=4.3)
    ap.add_argument("--att", type=float, default=2.0, help="dB/cm")
    ap.add_argument("--power", type=float, default=1e-3, help="W")
    ap.add_argument("--tscale", type=int, default=-13, help="engine timescale, log10(s)")
    ap.add_argument("--abstol", type=float, default=1e-8)
    ap.add_argument("--reltol", type=float, default=1e-4)
    ap.add_argument("--duration", type=float, default=None, help="s (default: pulses * 2 ns)")
    a = ap.parse_args()

    rng = random.Random(a.seed)
    N = a.N
    c = 299792458.0
    f0 = c / 1550e-9
    wls = [c / (f0 + (i - (a.nwl - 1) / 2) * 100e9) for i in range(a.nwl)]

    out = []
    w = out.append
    w("* CROW with %d rings, paper-style (ECOC 2023 Fig. 3); seed=%d nwl=%d" % (N, a.seed, a.nwl))
    vals = []
    for p in range(a.pulses):
        t_on = p * 2e-9
        t_off = t_on + 1e-9
        for wl in wls:
            vals.append("[%.12g,%.6g,%.12g]" % (t_on, a.power, wl))
        for wl in wls:
            vals.append("[%.12g,0,%.12g]" % (t_off, wl))
    w("VLSRC1 in values=[" + ",".join(vals) + "]")
    for i in range(N + 1):
        in1 = "in" if i == 0 else "r%da_out" % i
        in2 = "add" if i == N else "r%db_out" % (i + 1)
        out1 = "through" if i == 0 else "r%db_in" % i
        out2 = "drop" if i == N else "r%da_in" % (i + 1)
        w("COUPLER%d %s %s %s %s kp=%g" % (i, in1, in2, out1, out2, a.kp))
    for i in range(1, N + 1):
        la = max(1e-6, rng.gauss(a.lmean, a.lstd))
        lb = max(1e-6, rng.gauss(a.lmean, a.lstd))
        w("WG%da r%da_in r%da_out L=%.9g att=%g neff=%g ng=%g" % (i, i, i, la, a.att, a.neff, a.ng))
        w("WG%db r%db_in r%db_out L=%.9g att=%g neff=%g ng=%g" % (i, i, i, lb, a.att, a.neff, a.ng))
    if a.probes:
        w("PROBE1 through")
        w("PROBE2 drop")
    dur = a.duration if a.duration else a.pulses * 2e-9
    w(".options abstol=%g reltol=%g timescale=%d traceall=%d" % (a.abstol, a.reltol, a.tscale, a.traceall))
    w(".tran %g" % dur)
    print("\n".join(out))


if __name__ == "__main__":
    main()
