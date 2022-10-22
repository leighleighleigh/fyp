#!/usr/bin/env python3

import datetime
import json
import typing
from pathlib import Path
from time import time_ns
import matplotlib.pyplot as plt
import numpy as np

import pandas as pd
import pytz
from mcap.well_known import MessageEncoding, SchemaEncoding
from mcap.writer import Writer
from numpy import polyfit
# import kmeans
from sklearn.cluster import KMeans
from tslearn.clustering import TimeSeriesKMeans
from am4894plots.plots import plot_lines, plot_lines_grid

# Creates a Pandas Dataframe object from the json data
df = pd.read_json("merged.json", orient="records", lines=True)
# Replace index with the converted date_created_utc column
df['date_created_utc'] = pd.to_datetime(df['date_created_utc'])
df = df.set_index('date_created_utc')
eastern = pytz.timezone('Australia/Melbourne')
df.index = df.index.tz_localize(pytz.utc)
df.index = df.index.tz_convert(eastern)

# Using the value from the website for watermark reading
# Uses OHMS not kOhms
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


def convert_smt_vwc(df):
    """
    Converts raw SMT analog readings into a percentage Volumetric Water Content.
    Assumes 3.3V ADC max and 10-bit resolution.
    """
    return df.apply(lambda row: (row['smtVWC'] / 1023.0) * 3.3 * 100, axis=1)


def convert_smt_temp(df):
    """
    Converts raw SMT analog readings into a percentage Volumetric Water Content.
    Assumes 3.3V ADC max and 10-bit resolution.
    """
    return df.apply(lambda row: (((row['smtT'] / 1023.0) * 3.3 * 10)-4.0)*10, axis=1)

deviceprefix = ["C","S","P"]

for idx, gpx in enumerate(deviceprefix):
    device_group = df[df['id'].str[0] == gpx]

    # Iterate over individual devices
    for cidx, c in enumerate(device_group['id'].unique()):
        # g contains the data for a single column within this column group
        device = device_group[device_group['id'] == c]

        # Calculate average of Chameleon A/B channels, to derive resistance
        # Upper chameleon
        device['uCHAB'] = ((device['uCHA'] + device['uCHB'])/2)
        # Explicit notation for resistance conversion
        # device['uCHR'] = 10 * (1023.0 - device['uCHAB']) / device['uCHAB']
        # Alternate conversion using map function
        device['uCHR'] = device['uCHAB'].map(lambda x: 10 * (1023.0 - x) / x)
        # Lower chameleon
        device['lCHAB'] = ((device['lCHA'] + device['lCHB'])/2)
        # device['lCHR'] = 10 * (1023.0 - device['lCHAB']) / device['lCHAB']
        device['lCHR'] = device['lCHAB'].map(lambda x: 10 * (1023.0 - x) / x)


        def swapcolumns(a : str,b : str):
            device[a+"_"] = device[a].copy()
            device[b+"_"] = device[b].copy()
            device[a] = device[b+"_"].copy()
            device[b] = device[a+"_"].copy()

        # Some digital sensors are swapped around - this is observable by the 'leading peak' of the data.
        # As the column is filled from the top, it's natural that the top sensor will peak first.
        swapTempList = ["C2","P5","P4","C3","C4","C5","S2","S4","S5"]

        if c in swapTempList:
            swapcolumns("uT","lT")

        # Swap chameleons if they were wired incorrectly
        # Swaps the raw data (uCHA,uCHB,uCHAB,uCHR <=> lCHA,lCHB,lCHAB,lCHR)
        swapChameleonList = ["C1","C2","C3","C4","S2","S3","S4","P1","P5","S1","S5"]

        if c in swapChameleonList:
            swapcolumns("uCHA","lCHA")
            swapcolumns("uCHB","lCHB")
            swapcolumns("uCHAB","lCHAB")
            swapcolumns("uCHR","lCHR")

        # Ohms to Centibar
        device['uCB'] = apply_kohm_to_cb(device, chameleonKey='uCHR', tempKey='uT')
        device['lCB'] = apply_kohm_to_cb(device, chameleonKey='lCHR', tempKey='lT')

        device['uCB_nocal'] = apply_kohm_to_cb(device, chameleonKey='uCHR')
        device['lCB_nocal'] = apply_kohm_to_cb(device, chameleonKey='lCHR')
        
        # Convert to percentages
        device['smtVWC_pct'] = convert_smt_vwc(device)
        device['smtT_c'] = convert_smt_temp(device)

        # Remove zero-values
        def rmzero(key):
            device[key] = device[key][device[key]>0].copy()


        # Remove zeros from the data, which occur due to noise on the analog readings
        rmzero('smtVWC_pct')
        rmzero('smtT_c')
        rmzero('uCB')
        rmzero('lCB')
        rmzero('uCB_nocal')
        rmzero('lCB_nocal')


        # Smooth the data into 10 minute rolling averages
        smoothperiod = '30min'

        def smoothinline(key):
            return pd.Series(device[key]).rolling(smoothperiod,center=True).mean().copy()

        def smooth(key):
            device[key] = smoothinline(key)


        smooth('smtVWC_pct')
        smooth('smtT_c')
        smooth('uCB')
        smooth('lCB')
        smooth('uCB_nocal')
        smooth('lCB_nocal')
        smooth('uT')
        smooth('lT')


        # Calculate the rate of change of chmln_top_cb, w.r.t the time index
        device['uCB_rate'] = device['uCB'].diff() / device['uCB'].index.to_series().diff().dt.total_seconds()
        device['lCB_rate'] = device['lCB'].diff() / device['lCB'].index.to_series().diff().dt.total_seconds()

        # Convert the rate from cb/s to cb/h
        device['uCB_rate'] = device['uCB_rate'] * 3600
        device['lCB_rate'] = device['lCB_rate'] * 3600

        # Smooth!
        smooth('uCB_rate')
        smooth('lCB_rate')

