#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt

import circular_piston.impedance as cp_impedance
import spherical_baffle.impedance as sb_impedance
#import flanagan.impedance as fg_impedance
#import chalker.impedance as ch_impedance
import laine.impedance as la_impedance
import pole_zero_z.impedance as pz_impedance

T = 35 # deg C
FREQ_MIN = 100.0 # Hz
FREQ_MAX = 20e3 # Hz
FREQ_Z_MAX = 50e3 # Hz - this is equal to fs/2
FREQ_STEP = 50 # Hz
#A = 0.4e-2 # m (mouth radius)
A = 1.5e-2 # m (mouth radius)
A_SPHERE = 9e-2 # m (radius of spherical baffle)

N_CIRCULAR_PISTON = 40
#N_CIRCULAR_PISTON = 1
N_SPHERICAL_BAFFLE = 40

#==============================================================================

freq = np.arange(FREQ_MIN, FREQ_MAX, FREQ_STEP)
freq_z = np.arange(FREQ_MIN, FREQ_Z_MAX, FREQ_STEP)
c = 331.4 + (0.6 * T) # (m/s)

#w = 2.0 * np.pi * freq
#k = w / c
#two_ka = 2.0 * k * A

R_cp, X_cp = cp_impedance.impedance(freq, c, A,           N_CIRCULAR_PISTON)
R_sb, X_sb = sb_impedance.impedance(freq, c, A, A_SPHERE, N_SPHERICAL_BAFFLE)
#R_fg, X_fg = fg_impedance.impedance(freq, c, A)
#R_ch, X_ch = ch_impedance.impedance(freq, c, A)
freq_la, R_la, X_la = la_impedance.impedance(FREQ_STEP, A)
R_pz, X_pz = pz_impedance.impedance(freq_z, A)

#plt.figure()
#plt.plot(two_ka, R_cp, label="flat baffle")
#plt.plot(two_ka, R_sb, label="spherical baffle")
#plt.title("R(2ka)")
#plt.legend(loc="center right")
#plt.grid(True)
#
#plt.figure()
#plt.plot(two_ka, X_cp, label="flat baffle")
#plt.plot(two_ka, X_sb, label="spherical baffle")
#plt.title("X(2ka)")
#plt.legend(loc="center right")
#plt.grid(True)

plt.figure()
plt.plot(freq, R_cp, label="flat baffle")
plt.plot(freq, R_sb, label="spherical baffle")
#plt.plot(freq, R_fg, label="Flanagan")
#plt.plot(freq, R_ch, label="Chalker")
plt.plot(freq_la, R_la, label="Laine")
plt.plot(freq_z, R_pz, label="pole-zero Z")
plt.title("R(f)")
plt.legend(loc="center right")
plt.grid(True)

plt.figure()
plt.plot(freq, X_cp, label="flat baffle")
plt.plot(freq, X_sb, label="spherical baffle")
#plt.plot(freq, X_fg, label="Flanagan")
#plt.plot(freq, X_ch, label="Chalker")
plt.plot(freq_la, X_la, label="Laine")
plt.plot(freq_z, X_pz, label="pole-zero Z")
plt.title("X(f)")
plt.legend(loc="center right")
plt.grid(True)

plt.show()
