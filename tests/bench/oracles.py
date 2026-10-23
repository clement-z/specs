#!/usr/bin/env python3
"""oracles.py: closed-form steady-state responses of ring circuits, in SPECS's conventions.

Device models (the equations of src/devices/waveguide.cpp and directional_coupler.cpp):
    waveguide   E_out = a e^{j phi} E_in,   a = 10^(-att_dB_cm * L_cm / 20),
                phi = 2 pi neff(lambda) L / lambda,  neff(lambda) = neff0 + (neff0 - ng0) / lambda_ref (lambda - lambda_ref),
                lambda_ref = 1.55 um (no dispersion term D)
    coupler     out1 = t in1 + j kappa in2,  out2 = j kappa in1 + t in2  (cross phase pi/2),  t = sqrt(1 - kappa^2) (lossless);
                netlist k = kappa (field), kp = kappa^2 (power)

Oracles:
    add_drop(...)   all-pass/add-drop ring (Yariv 2000, 2002; Bogaerts et al. 2012, eq. for T(r, a, phi)):
                    through/in = (t1 - t2 A e^{j phi}) / (1 - t1 t2 A e^{j phi}),
                    drop/in    = -kappa1 kappa2 sqrt(A) e^{j phi/2} / (1 - t1 t2 A e^{j phi})
                    (two equal half rings, A = a_half^2, phi = 2 phi_half)
    crow(...)       N-ring CROW by transfer matrices (Poon et al. 2004): the field pair on each side of a
                    coupler is propagated from the drop end (drop = 1, add = 0) back to the input, through
                    each ring's two halves; through/in and drop/in follow. Handles unequal halves (the
                    paper's random ring lengths) and N = 1 (equal to add_drop).

`Model` holds the physics switches the mutation checks of test_oracles.py flip (k read as a power ratio,
no dispersion slope, a real cross-coupling coefficient): a correct SPECS must match the default model and
miss every mutated one.
"""
import cmath
import math

LAMBDA_REF = 1.55e-6


class Model:
    """Device equations of SPECS; the keyword arguments inject the physics errors of the mutation checks."""

    def __init__(self, k_as_power=False, no_dispersion_slope=False, cross_phase_rad=math.pi / 2):
        self.k_as_power = k_as_power
        self.no_dispersion_slope = no_dispersion_slope
        self.cross_phase_rad = cross_phase_rad

    def waveguide(self, lam, length_m, neff0, ng0, att_db_cm):
        a = 10 ** (-(att_db_cm * length_m * 100.0) / 20.0)
        slope = 0.0 if self.no_dispersion_slope else (neff0 - ng0) / LAMBDA_REF
        neff = neff0 + slope * (lam - LAMBDA_REF)
        return a * cmath.exp(1j * 2.0 * math.pi * neff * length_m / lam)

    def coupler(self, kappa):
        """(through, cross) field coefficients of a coupler of field cross-coupling kappa.

        (Flipping +j to -j in every coupler multiplies drop/in by (-1)^(number of couplers) and leaves
        through/in unchanged, a path into a ring and back crossing twice: invisible in circuits/add_drop.cir
        and in a CROW with an even number of couplers, so it is not one of the mutants.)
        """
        if self.k_as_power:
            kappa = kappa * kappa
        t = math.sqrt(1.0 - kappa * kappa)
        return complex(t, 0.0), cmath.rect(kappa, self.cross_phase_rad)


SPECS = Model()


def add_drop(lam, kappa1, kappa2, half_length_m, neff0, ng0, att_db_cm, model=SPECS):
    """(through/in, drop/in) of an add-drop ring with two equal halves (closed form).

    Coupler 1: in1 = in, in2 = ring return, out1 = through, out2 = ring forward; coupler 2 takes the ring
    after one half and gives the drop port (circuits/add_drop.cir). With u the field entering the ring:
    u = x1 in / (1 - t1 t2 h^2), through = t1 in + x1 t2 h^2 u, drop = x2 h u (h: one half).
    """
    t1, x1 = model.coupler(kappa1)
    t2, x2 = model.coupler(kappa2)
    h = model.waveguide(lam, half_length_m, neff0, ng0, att_db_cm)
    den = 1.0 - t1 * t2 * h * h
    # t1^2 - x1^2 = t1^2 + kappa1^2 = 1 (lossless coupler): Yariv's (t1 - t2 A e^{j phi}) / (1 - t1 t2 A e^{j phi})
    through = (t1 - (t1 * t1 - x1 * x1) * t2 * h * h) / den
    drop = x1 * x2 * h / den
    return through, drop


def crow(lam, kappas, halves, neff0, ng0, att_db_cm, model=SPECS):
    """(through/in, drop/in) of a CROW: len(kappas) = N + 1 couplers, halves = [(L_ia, L_ib)] for N rings.

    Ring i: half a from coupler i-1 out2 to coupler i in1, half b from coupler i out1 to coupler i-1 in2
    (the topology of tests/bench/crow_gen.py and src/devices/crow.cpp).
    """
    n = len(halves)
    assert len(kappas) == n + 1 and n >= 1
    # Right side of the last coupler: out2 = drop = 1, in2 = add = 0
    b2, a2 = 1.0 + 0j, 0j
    for i in range(n, -1, -1):
        t, x = model.coupler(kappas[i])
        # out2 = x in1 + t in2, out1 = t in1 + x in2  ->  in1, out1 from out2, in2
        a1 = (b2 - t * a2) / x
        b1 = t * a1 + x * a2
        if i == 0:
            return b1 / a1, 1.0 / a1
        la, lb = halves[i - 1]
        ha = model.waveguide(lam, la, neff0, ng0, att_db_cm)
        hb = model.waveguide(lam, lb, neff0, ng0, att_db_cm)
        # in1 of coupler i = ha * out2 of coupler i-1; in2 of coupler i-1 = hb * out1 of coupler i
        b2, a2 = a1 / ha, hb * b1
    raise AssertionError("unreachable")
