#!/usr/bin/env python3
"""Closed-form check of circuits/add_drop.cir (CW sweep) against the add-drop ring transfer function.

  mkdir -p traces && specs -f circuits/add_drop.cir -o traces/add_drop
  python3 -I a5-adddrop-analytic.py traces/add_drop.vcd

Model (netlist values: field coupling k=0.85, 2 dB/cm, neff=2.3, ng=4.3, half-ring L=101*lambda0/neff, lambda0=1.55um;
waveguide phase = 2*pi*L*neff(l)/l with neff(l)=neff+(neff-ng)/1.55e-6*(l-1.55e-6), see src/devices/waveguide.cpp):
  h = t*exp(j*phi)                (one half ring)
  T_through = r - k^2 r h^2 /(1 - r^2 h^2)
  T_drop    = -k^2 h /(1 - r^2 h^2),  r = sqrt(1-k^2)
Observed on d9eb1e6+: max |P_sim - P_model| = 1.4e-6 W over the 30 swept points (netlist reltol=1e-6) -> suggested tolerance 1e-5.
"""
import sys, cmath, math

def read_series(path, wanted):
    ids, scope, out = {}, [], {}
    t, defs = 0, True
    with open(path) as f:
        for line in f:
            line = line.strip()
            if defs:
                if line.startswith('$scope'): scope.append(line.split()[2])
                elif line.startswith('$upscope'): scope.pop()
                elif line.startswith('$var'):
                    p = line.split(); name = '.'.join(scope) + '.' + p[4]
                    if name in wanted: ids.setdefault(p[3], []).append(name)
                elif line.startswith('$enddefinitions'): defs = False
                continue
            if line.startswith('#'): t = int(line[1:])
            elif line.startswith('r'):
                v, i = line[1:].split()
                for n in ids.get(i, []): out.setdefault(n, []).append((t, float(v)))
    return out

names = ['SystemC.PROBE{ROOT/out}.power', 'SystemC.PROBE{ROOT/drop}.power', 'SystemC.PROBE{ROOT/out}.wavelength']
s = read_series(sys.argv[1], names)
k, neff, ng, att, lam0 = 0.85, 2.3, 4.3, 2.0, 1.55e-6
L = 101 * lam0 / neff
r = math.sqrt(1 - k * k)
worst = 0.0
for (_, lam), (_, po), (_, pd) in zip(s[names[2]], s[names[0]], s[names[1]]):
    ne = neff + (neff - ng) / 1.55e-6 * (lam - 1.55e-6)
    h = 10 ** (-att * L * 100 / 20) * cmath.exp(1j * 2 * math.pi * L * ne / lam)
    den = 1 - r * r * h * h
    pt, pdrop = abs(r - k * k * r * h * h / den) ** 2, abs(-k * k * h / den) ** 2
    worst = max(worst, abs(po - pt), abs(pd - pdrop))
print('points: %d, max |P_sim - P_model| = %.3g W' % (len(s[names[2]]), worst))
sys.exit(0 if worst < 1e-5 else 1)
