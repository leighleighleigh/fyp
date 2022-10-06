#!/usr/bin/env python

# ECE4701 - Sensors In Green Infrastructure, Semester 1

# Loads a pickled data frame of some chameleon CSV data.
# Plots it all together

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import matplotlib.ticker as mticker
import datetime
import pandas as pd
import numpy as np
import sys
import fire
import math

# Set colours
plt.style.use('seaborn-pastel')  
mpl.rcParams['figure.figsize'] = (8, 6)
mpl.rcParams['lines.linewidth'] = 4


# Using the value from the website for watermark reading
# Uses OHMS not kOhms
def ohm_to_cb(rS):
    tempC = 24
    if rS < 550:
        return 0
    if rS < 1000:
        return abs((rS/1000*23.156-12.736)*-(1+0.018*(tempC-24)))
    if rS < 8000:
        return abs((-3.213*(rS/1000.00)-4.093)/(1-0.009733*(rS/1000.00)-0.01205*(tempC)))
    
    if rS < 35000:
        return abs(-2.246-5.239*(rS/1000.00)*(1+.018*(tempC-24.00))-.06756*(rS/1000.00)*(rS/1000.00)*((1.00+0.018*(tempC-24.00))*(1.00+0.018*(tempC-24.00))))

# Open-circuited!
    return 255

# Grab filename
logFile = {"file":"esp32s2_cluster_pottingsoil_wetter_again_cleaned.csv","date":"19-05-2022 17:12","submerged":0.0}
#logFile = {"file":"chameleon_after_soak_dryingout_1711190522.csv","date":"19-05-2022 17:12","submerged":0.0}
# Read csv, index is auto-generated 
df = pd.read_csv(logFile['file'])
# Create a starting datetime (this will be file-dependent, manually entered)
startTime = pd.to_datetime(logFile["date"],infer_datetime_format=True)
# Convert the 'time_s' column into datetimes - using startTime as the origin
df['time_ms'] = pd.to_datetime(df['time_ms'], unit="ms", origin=startTime)
# Set the index to be time_s
df = df.set_index('time_ms')

# Clean Temperature data of erraneous values
#df.loc[df['tempC'] == -127,'tempC'] = np.nan

# Create a new rolling-median column from the raw resistane data
#df['rS_rolling_median'] = df['rS'].rolling(pd.Timedelta("5 minute")).median()
df['rS_rolling_mean'] = df['chmn_b_r'].rolling(pd.Timedelta("5 minute")).mean()
#df['rS_corrected_rolling_median'] = df['rS_corrected'].rolling(pd.Timedelta("5 minute")).median()
#df['rS_corrected_rolling_mean'] = df['rS_corrected'].rolling(pd.Timedelta("5 minute")).mean()
#df['tempC_rolling_mean'] = df['tempC'].rolling(pd.Timedelta("5 minute")).mean()

# Create the kPa chart!
df['kPa'] = df['rS_rolling_mean'].map(ohm_to_cb)

# Setup multiple subplots
fig,axes = plt.subplots(1,1,sharex=False)
# Extract the axis for each plot
resistancePlot = axes

## Plot the raw sensor data
#df.plot(ax=resistancePlot,y="rS_rolling_mean",label='Resistance')
df.plot(ax=resistancePlot,y="kPa",label='Tension')

# Nice legends next to the plot
def addLegend(ax):
    # Shrink current axis by 20%
    box = ax.get_position()
    ax.set_position([box.x0, box.y0, box.width * 0.75, box.height])
    # Put a legend to the right of the current axis
    ax.legend(loc='center left', bbox_to_anchor=(1, 0.5), fancybox=True, shadow=True)

fontS = 12

#resistancePlot.set_ylim(bottom=0,top=1000)
#resistancePlot.set_xlim(right=pd.Timestamp(2022,5,20,12))
#resistancePlot.set_ylabel("Resistance $\Omega$")
#resistancePlot.set_xlabel("")
#resistancePlot.set_title("Range of ACAIR calibration",fontsize=fontS)

fig.suptitle("Chameleon Sensor\n kPa conversion",fontsize=16)
plt.tight_layout()
plt.show()
