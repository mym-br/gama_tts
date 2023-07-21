# This file is in the public domain.

# Pole-zero in Z domain.
#
# fs must be > 50 kHz, but not much higher than 100 kHz.

# Sources:
#
# Liljencrants - "Speech synthesis with a reflection-type line analog", 1985.
#
# Laine - "Modelling of lip radiation impedance in z-domain", 1992

import numpy as np

_TRANSITION_RADIUS = 0.5e-2

# Z.real = 0.5 at transition frequency.
def transition_freq(a):
    # Spherical baffle with 9 cm radius.
    a = max(a, _TRANSITION_RADIUS) # minimum radius
    return 62.33711741947817 / a + 320.20420449105177



# Return impedance divided by (density * c).
#
# Z = 1 at fs/2.
def impedance(freq, a):
    fs = 2.0 * freq[-1]
    T = 1.0 / fs
    w = 2.0 * np.pi * freq

    cos_wt = np.cos(2.0 * np.pi * transition_freq(a) * T)

    qa = 2.0 * cos_wt
    qb = -2.0 * (cos_wt + 1.0)
    qc = cos_wt + 1.0
    delta = qb**2 - 4.0 * qa * qc
    ca = (-qb - np.sqrt(delta)) / (2.0 * qa)
    #ca = (-qb + np.sqrt(delta)) / (2.0 * qa)

    cb = 2.0 * ca - 1.0

    if a < _TRANSITION_RADIUS:
        ca *= 40391.175581408956 * a**2

    z = np.exp(1j * w * T)
    Z = ca * (z - 1.0) / (z - cb)

    return Z.real, Z.imag
