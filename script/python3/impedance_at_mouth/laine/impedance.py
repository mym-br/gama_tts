# This file is in the public domain.

# Laine.

# Sources:
#
# Liljencrants - "Speech synthesis with a reflection-type line analog", 1985.
#
# Laine - "Modelling of lip radiation impedance in z-domain", 1992

import numpy as np

# Return impedance divided by (density * c).
# a <= 1.6e-2
# The error is minimized between 0 and 5 kHz.
def impedance(freq_step, a):
    fs = 20000.0
    freq = np.arange(0.0, 0.5 * fs, freq_step)

    T = 1.0 / fs
    area = np.pi * a**2
    w = 2.0 * np.pi * freq

    z = np.exp(1j * w * T)

    sqrt_area = np.sqrt(area * 1e4) # convert to cm^2
    ca = 0.0779 + 0.2373 * sqrt_area
    cb = -0.8430 + 0.3062 * sqrt_area
    Z = ca * (1.0 - 1.0 / z) / (1.0 - cb / z)

    return freq, Z.real, Z.imag
