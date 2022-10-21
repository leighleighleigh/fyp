#!/usr/bin/python3

import numpy as np
import matplotlib.pyplot as plt

def ohm_to_cb(rS, tempC):
    tempCalib = 24

    if rS < 550:
        return 0

    if rS < 1000:
        return abs(((rS/1000)*23.156-12.736)*-(1+0.018*(tempC-tempCalib)))

    if rS < 8000:
        return abs((-3.213*(rS/1000.0)-4.093)/(1-0.009733*(rS/1000.0)-0.01205*(tempC)))

    if rS < 35000:
        return abs(-2.246-5.239*(rS/1000.00)*(1+.018*(tempC-tempCalib))-.06756*(rS/1000.00)*(rS/1000.00)*((1.00+0.018*(tempC-tempCalib)*(1.00+0.018*(tempC-tempCalib)))))

    # Open-circuited!
    return 255

x = np.linspace(0, 35000, 1000)

# Iterate over temperature points, generating series for each
temps = np.linspace(12,30,10)

for temp in temps:
    y = np.array([ohm_to_cb(r, temp) for r in x])
    plt.plot(x, y, label="{}C".format(temp))

# plt.plot(x, y)
plt.legend()
plt.show()