#!/usr/bin/env python3

import matplotlib
matplotlib.use('WebAgg')
import matplotlib.pyplot as plt
import pandas as pd
import json
import pytz

plt.style.use('seaborn')
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

with open('merged_array.json', 'w') as f_out:
    f_out.write(json.dumps(data))

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

# Group these names based on prefix, which is the first letter
deviceprefix = ['C','P','S']

# plt.figure(figsize=(6,4),dpi=150)
fig, axs = plt.subplots(3,1,sharex=True)

for idx,gpx in enumerate(deviceprefix):
  groupdf = df[df['device_name'].str[0] == gpx]
  # groupdf = groupdf.pivot_table(index=['device_name','date_created_utc'],values='smt_vwc_raw')
  groupdf = groupdf.pivot_table(index=['device_name','date_created_utc'],values=['chmln_top_ohms','chmln_bot_ohms'])
  # groupdf = groupdf.pivot_table(index=['device_name','date_created_utc'],values=['chmln_bot_ohms'])
  groupdfs = groupdf.unstack(level=0)

  ax = axs[idx]

  for cidx,c in enumerate(groupdfs.columns.values.tolist()):
    rawbat = groupdfs[c]
    # Fill in gaps linearly, showing the raw data as a thin dotted line
    linbat = pd.Series(rawbat).interpolate(method='time',limit_direction='forward',limit_area='inside')
    # Derive the rolling median from the battery data, plotting it as a thicker line with the same colour
    avgbat = pd.Series(rawbat).rolling('1h').mean()
    _linecolor = colors[cidx % len(colors)]
    # linbat.plot(ax=ax,sharex=True,lw=1,ls='--')
    avgbat.plot(ax=ax,sharex=True,style=[_linecolor],lw=1,ls='-')
    ax.autoscale(True,axis='y',tight=True)
    ax.legend()
    ax.set_ylim(0,6000)

  # plt.title(gpx)
  # plt.xlabel('Time')
  # plt.legend()

plt.tight_layout()
plt.show()
