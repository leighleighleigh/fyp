#!/usr/bin/env python3

import matplotlib.pyplot as plt
import pandas as pd
import json
import pytz

#plt.style.use('seaborn')
prop_cycle = plt.rcParams['axes.prop_cycle']
colors = prop_cycle.by_key()['color']

data = []

# Opens the per-line-JSON file, merged from get_merged_logs.sh script
with open('merged.json', 'r') as f_in:
    for line in f_in:
        try:
            data.append(json.loads(line))
        except:
            continue    # ignore the error

# Creates a Pandas Dataframe object from the json data
df = pd.DataFrame(data)

def unpack(x):
    rv = []
    for v in x:
        if isinstance(v, dict):
            rv.append([*v.values()][0])
        else:
            rv.append(v)
    return rv

df = df.apply(unpack)
print(df)

# Replace index with the converted date_created_utc column
df['date_created_utc']=pd.to_datetime(df['date_created_utc'])
df=df.set_index('date_created_utc')
print(df)
# Convert index to australian timezone (from UTC)
eastern = pytz.timezone('Australia/Melbourne')
df.index = df.index.tz_localize(pytz.utc)
print(df)
df.index = df.index.tz_convert(eastern)
print(df)

def plot_ensemble_data(df, key='batt_mv', window='1h'):
  # Now we can plot the battery of all boards at once!
  plt.figure(figsize=(6, 4), dpi=150)

  # Pivot the data so we can query on a per-board basis
  # Groups by device name, date created, and key
  boards = df.pivot_table(index=['device_name','date_created_utc'], values=key)
  # New dataframe where index is date_created, columns are the board names, and the values are batt_mv
  boards_batt_mv = boards.unstack(level=0)


  for cidx,c in enumerate(boards_batt_mv.columns.values.tolist()):
    rawbat = boards_batt_mv[c]
    # Fill in gaps linearly, showing the raw data as a thin dotted line
    linbat = pd.Series(rawbat).interpolate(method='time',limit_direction='forward',limit_area='inside')
    # Derive the rolling median from the battery data, plotting it as a thicker line with the same colour
    avgbat = pd.Series(rawbat).rolling('1h').mean()
    _linecolor = colors[cidx % len(colors)]
    linbat.plot(lw=1,ls='--')
    avgbat.plot(style=[_linecolor],lw=3,ls='-')

  plt.title(key)
  plt.xlabel('Time')
  plt.legend()

plot_ensemble_data(df,'batt_mv')
#plot_ensemble_data(df,'uptime_s')
plot_ensemble_data(df,['chmln_top_ohms','chmln_bot_ohms'])
plot_ensemble_data(df,['smt_vwc_raw'])
plt.show()

