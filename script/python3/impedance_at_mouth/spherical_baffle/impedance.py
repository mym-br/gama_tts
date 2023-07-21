# This file is in the public domain.

# Circular piston in spherical baffle.

# Sources:
#
# Chalker, Mackerras - "Models for Representing the Acoustic Radiation
# Impedance of the Mouth", 1985.
#
# Morse, Ingard - "Theoretical Acoustics", 1968.

# Note:
# - spherical_yn returns -inf for z = 0. This happens when the input frequency
#   is zero.

import numpy as np
from scipy.special import spherical_jn, spherical_yn, lpn

def calc_R_X(kas, phi, n):
    R = np.zeros(kas.shape)
    X = np.zeros(kas.shape)
    kas_2 = kas**2
    cos_phi = np.cos(phi)

    P = lpn(n, cos_phi)[0]

    for m in np.arange(n):
        P_p = P[m + 1]
        if m == 0:
            P_n = 1.0 # from Morse-1968
        else:
            P_n = P[m - 1]

        jn_p = spherical_jn(m + 1, kas)
        if m > 0:
            jn_n = spherical_jn(m - 1, kas)
        else:
            jn_n = np.zeros(kas.shape)
        yn_p = spherical_yn(m + 1, kas)
        if m > 0:
            yn_n = spherical_yn(m - 1, kas)
        else:
            yn_n = np.zeros(kas.shape)

        B = ((1.0 / (2.0 * m + 1.0))
             * np.sqrt((m * yn_n - (m + 1) * yn_p)**2
                       + ((m + 1) * jn_p - m * jn_n)**2))
        delta = np.arctan2((m + 1) * jn_p - m * jn_n,
                           m * yn_n - (m + 1) * yn_p)

        R += (P_n - P_p)**2 / (kas_2 * (2.0 * m + 1.0) * (B * np.sin(phi * 0.5))**2)

        X += (((P_n - P_p)**2 / ((2.0 * m + 1.0) * B * np.sin(phi * 0.5)**2))
              * (spherical_jn(m, kas) * np.sin(delta) - spherical_yn(m, kas) * np.cos(delta)))

    return 0.25 * R, 0.25 * X

# Return impedance divided by (density * c).
def impedance(freq, c, a, a_sphere, n):
    w = 2.0 * np.pi * freq
    k = w / c
    phi = np.arcsin(a / a_sphere)

    R, X = calc_R_X(k * a_sphere, phi, n)
    return R, X
