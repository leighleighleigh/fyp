#!/usr/bin/env python3

"""
Functions here are used to convert from RAW sensor data,
such as from Chameleon, DS18B20, and SMT100 sensors,
into SI-unit values.

Functions here are used in the 'merged_json_to_csv' file.
"""

import math
import pandas as pd
import numpy as np

# Convert two raw chameleon readings (channel A and B), and convert it to an ohms value
def chameleon_raw_to_ohms(rawA, rawB) -> float:
    rawAvg = (rawA+rawB)/2
    return 10*((1023.0 - rawAvg)/rawAvg)*1000

# Using the value from the website for watermark reading
# Uses OHMS not kOhms
def ohm_to_cb(rS, tempC, ignore_range=(0,100)):
    result = np.nan # Default result, unclamped value, which indicates an error in the reading
    tempCalib = 24

    if rS < 550:
        result = 0

    elif rS < 1000:
        result = abs(((rS/1000)*23.156-12.736)*-(1+0.018*(tempC-tempCalib)))

    elif rS < 8000:
        result = abs((-3.213*(rS/1000.0)-4.093)/(1-0.009733*(rS/1000.0)-0.01205*(tempC)))

    elif rS < 35000:
        result = abs(-2.246-5.239*(rS/1000.00)*(1+.018*(tempC-tempCalib))-.06756*(rS/1000.00)*(rS/1000.00)*((1.00+0.018*(tempC-tempCalib)*(1.00+0.018*(tempC-tempCalib)))))

    if result >= ignore_range[0] and result <= ignore_range[1]:
        return result

    return np.nan

def apply_kohm_to_cb(df, chameleonKey='chmln_top_ohms', tempKey=None):
    """
    Operates on two input series, df_chameleon and df_temp, and derives a new value via ohm_to_cb.
    Operates over the entire series at once.
    """
    # If tempKey is not provided, assumes tempC=24
    if tempKey is None:
        return df.apply(lambda row: ohm_to_cb(row[chameleonKey]*1000, 24), axis=1)
    else:
        return df.apply(lambda row: ohm_to_cb(row[chameleonKey]*1000, row[tempKey]), axis=1)


def smt_raw_to_vwc(rawVWC):
    """
    Converts raw SMT analog readings into a percentage Volumetric Water Content.
    Assumes 3.3V ADC max and 10-bit resolution.
    """
    return (rawVWC / 1023.0) * 3.3 * 100


def smt_raw_to_temp(rawTemp):
    """
    Converts raw SMT analog readings into a percentage Volumetric Water Content.
    Assumes 3.3V ADC max and 10-bit resolution.
    """
    return (((rawTemp / 1023.0) * 3.3 * 10)-4.0)*10
