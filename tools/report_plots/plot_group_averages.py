#!/usr/bin/env python3

# Loads the column data from CSV file, and plots the group averages.
import pandas as pd
import matplotlib.pyplot as plt

# Load the data from the CSV file.
df = pd.read_csv('plot_data.csv')
# Convert column 'elapsed_time' into datetime object
# This column comes from Foxglove CSV plot export.
df['time'] = pd.to_datetime(df['receive time'],unit='s')

# Pivot the data by the 'topic' column
df = df.pivot(index='time', columns='topic', values='value')

# Remape value '/group_C_avg.uCB' to 'Group C Upper Water Tension (CB)'
df = df.rename(columns={'/group_C_avg.uCB': 'Control Group'})
df = df.rename(columns={'/group_S_avg.uCB': 'Surface Clogging Group'})
df = df.rename(columns={'/group_P_avg.uCB': 'Preferential Flow Group'})

# Limit index to a datetime range
df = df.loc[df.index[0]:df.index[0]+pd.Timedelta('12:00:00')]

# Plot the data
ax = df.plot()
ax.set_ylabel('Water Tension (Centibar)')
ax.set_xlabel('Time')
ax.set_title('Group Averages, 12 Hours following dosage, Upper Sensors')
ax.legend(loc='upper left')
ax.grid(True)
plt.show()