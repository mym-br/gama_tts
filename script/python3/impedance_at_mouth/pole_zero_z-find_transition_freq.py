#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

import spherical_baffle.impedance as sb_impedance

T = 35 # deg C
FREQ_MIN = 100.0 # Hz
FREQ_MAX = 20e3 # Hz
FREQ_STEP = 5 # Hz
MIN_A = 0.32e-2 # m (mouth radius)
MAX_A = 5.0e-2 # m (mouth radius)
A_STEP = 0.1e-2
A_STEP_FIT = 0.02e-2
A_SPHERE = 9e-2 # m (radius of spherical baffle)

N_SPHERICAL_BAFFLE = 40

#==============================================================================

freq = np.arange(FREQ_MIN, FREQ_MAX, FREQ_STEP)
c = 331.4 + (0.6 * T) # (m/s)

a_list = np.arange(MIN_A, MAX_A, A_STEP)

freq_list = np.zeros(a_list.shape)

for i, a in enumerate(a_list):
    print("i={}/{}".format(i, len(a_list) - 1))
    R_sb, X_sb = sb_impedance.impedance(freq, c, a, A_SPHERE, N_SPHERICAL_BAFFLE)
    trans_freq_idx = np.where(R_sb >= 0.5)[0][0]
    freq_list[i] = freq[trans_freq_idx]

#------------------------------------------------------------------------------
# Curve fitting.

def f(x, a0, a1):
    return a0 / x + a1

popt, pcov = curve_fit(f, a_list, freq_list)
print("f constants: {} {}".format(*popt))
#------------------------------------------------------------------------------

plt.figure()
a_list_fit = np.arange(MIN_A, MAX_A, A_STEP_FIT)
plt.plot(a_list, freq_list, "x")
plt.plot(a_list_fit, f(a_list_fit, *popt))
plt.xlabel("mouth radius (m)")
plt.ylabel("transition freq. (Hz)")
plt.grid(True)

plt.show()
