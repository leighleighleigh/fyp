#!/usr/bin/env python3

# Loads the column data from CSV file, and plots the group averages.
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl

# Set colours
# plt.style.use('seaborn-pastel')  
mpl.rcParams['figure.figsize'] = (6, 4)
mpl.rcParams['lines.linewidth'] = 4

# Load the data from the CSV file.
df = pd.read_csv('plot_data2.csv')
# Convert column 'elapsed_time' into datetime object
# This column comes from Foxglove CSV plot export.
df['time'] = pd.to_datetime(df['receive time'],unit='s')

# Pivot the data by the 'topic' column
df = df.pivot(index='time', columns='topic', values='value')

# Remape value '/group_C_avg.uCB' to 'Group C Upper Water Tension (CB)'
df = df.rename(columns={'/group_C_avg.lCB': 'Control Group'})
df = df.rename(columns={'/group_S_avg.lCB': 'Surface Clogging Group'})
df = df.rename(columns={'/group_P_avg.lCB': 'Preferential Flow Group'})

# Limit index to a datetime range
df = df.loc[df.index[0]:df.index[0]+pd.Timedelta('12:00:00')]

# Plot the data
ax = df.plot()
ax.set_ylabel('Water Tension (Centibar)')
ax.set_xlabel('Time')
# Set y-range to be consistent
ax.set_ylim(0, 100)
ax.set_title('Group Averages, 12 Hours following dosage, Lower Sensors')
ax.legend(loc='upper left')
ax.grid(True)
# plt.show()
plt.savefig('lower_results.png',dpi=150)