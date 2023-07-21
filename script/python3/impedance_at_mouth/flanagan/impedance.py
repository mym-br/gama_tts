# This file is in the public domain.

# Flanagan.

# Source:
#
# Chalker, Mackerras - "Models for Representing the Acoustic Radiation
# Impedance of the Mouth", 1985.

import numpy as np

# Return impedance divided by (density * c).
def impedance(freq, c, a):
    w = 2.0 * np.pi * freq
    k = w / c
    ka = k * a
    Z = (ka**2 / 2.0 + 1j * 8.0 * ka / (3.0 * np.pi))
    return Z.real, Z.imag
