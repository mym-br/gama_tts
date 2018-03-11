# Circular piston in infinite flat baffle.

import numpy as np

def calc_R(x_list, n):
    res = np.zeros(x_list.shape)
    for i, x in enumerate(x_list):
        sum = 0.0
        d = 1.0
        for j in np.arange(n):
            if j % 2 == 0:
                s = 1.0
            else:
                s = -1.0
            p = (j + 1) * 2
            d *= p * (p + 2)
            sum += s * (x**p) / d
        res[i] = sum
    return res

def calc_X(x_list, n):
    res = np.zeros(x_list.shape)
    for i, x in enumerate(x_list):
        sum = 0.0
        d = 1.0
        for j in np.arange(n):
            if j % 2 == 0:
                s = 1.0
            else:
                s = -1.0
            p = 1 + j * 2
            d *= p * (p + 2)
            sum += s * (x**p) / d
        res[i] = sum
    return (4.0 / np.pi) * res

# Return impedance divided by (density * c).
def impedance(freq, c, a, n):
    w = 2.0 * np.pi * freq
    k = w/c
    two_ka = 2.0 * k * a
    R = calc_R(two_ka, n)
    X = calc_X(two_ka, n)
    return R, X
