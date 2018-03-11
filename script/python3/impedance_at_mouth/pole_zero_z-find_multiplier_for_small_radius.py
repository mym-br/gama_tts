#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

import spherical_baffle.impedance as sb_impedance

T = 35 # deg C
FREQ = 5e3 # Hz
MIN_A = 0.01e-2 # m (mouth radius)
MAX_A = 0.51e-2 # m (mouth radius)
A_STEP = 0.01e-2

A_SPHERE = 9e-2 # m (radius of spherical baffle)

N_SPHERICAL_BAFFLE = 40

#==============================================================================

freq = np.array([FREQ])
c = 331.4 + (0.6 * T) # (m/s)

a_list = np.arange(MIN_A, MAX_A, A_STEP)

R_sb_list = np.zeros(a_list.shape)

for i, a in enumerate(a_list):
    print("i={}/{}".format(i, len(a_list) - 1))
    R_sb, X_sb = sb_impedance.impedance(freq, c, a, A_SPHERE, N_SPHERICAL_BAFFLE)
    R_sb_list[i] = R_sb[0]

amp_list = R_sb_list / R_sb_list[-1]

#------------------------------------------------------------------------------
# Curve fitting.

def f(x, a0):
    return a0 * x**2

popt, pcov = curve_fit(f, a_list, amp_list)
print("f constant: {}".format(*popt))
#------------------------------------------------------------------------------

plt.figure()
plt.plot(a_list, amp_list, ".")
plt.plot(a_list, f(a_list, *popt))
plt.xlabel("mouth radius (m)")
plt.ylabel("amplitude at " + str(FREQ) + " Hz")
plt.grid(True)

plt.show()
