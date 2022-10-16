#!/usr/bin/env python3

import pytz
import json
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl
import matplotlib
matplotlib.use('WebAgg')

plt.style.use('seaborn-pastel')
mpl.rcParams['figure.figsize'] = (8, 6)
mpl.rcParams['lines.linewidth'] = 4
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
df['date_created_utc'] = pd.to_datetime(df['date_created_utc'])
df = df.set_index('date_created_utc')
print(df)
# Convert index to australian timezone (from UTC)
eastern = pytz.timezone('Australia/Melbourne')
df.index = df.index.tz_localize(pytz.utc)
print(df)
df.index = df.index.tz_convert(eastern)
print(df)

# Using the value from the website for watermark reading
# Uses OHMS not kOhms


def ohm_to_cb(rS, tempC):
    tempCalib = 24

    if rS < 550:
        return 0

    if rS < 1000:
        return abs((rS/1000*23.156-12.736)*-(1+0.018*(tempC-tempCalib)))

    if rS < 8000:
        return abs((-3.213*(rS/1000.00)-4.093)/(1-0.009733*(rS/1000.00)-0.01205*(tempC)))

    if rS < 35000:
        return abs(-2.246-5.239*(rS/1000.00)*(1+.018*(tempC-tempCalib))-.06756*(rS/1000.00)*(rS/1000.00)*((1.00+0.018*(tempC-tempCalib)*(1.00+0.018*(tempC-tempCalib)))))

    # Open-circuited!
    return 255


def apply_ohm_to_cb(df, chameleonKey='chmln_top_ohms', tempKey='ds18b20_top_temp_c'):
    """
    Operates on two input series, df_chameleon and df_temp, and derives a new value via ohm_to_cb.
    Operates over the entire series at once.
    """
    return df.apply(lambda row: ohm_to_cb(row[chameleonKey], row[tempKey]), axis=1)

def convert_smt_vwc(df):
    """
    Converts raw SMT analog readings into a percentage Volumetric Water Content.
    Assumes 3.3V ADC max and 10-bit resolution.
    """
    return df.apply(lambda row: (row['smt_vwc_raw'] / 1023.0) * 3.3 * 100, axis=1)

def convert_smt_temp(df):
    """
    Converts raw SMT analog readings into a percentage Volumetric Water Content.
    Assumes 3.3V ADC max and 10-bit resolution.
    """
    return df.apply(lambda row: (((row['smt_temp_raw'] / 1023.0) * 3.3 * 10)-4.0)*10, axis=1)

# Group these names based on prefix, which is the first letter
deviceprefix = ['C', 'P', 'S']
# fig, axs = plt.subplots(3, 1, sharex=True)

for idx, gpx in enumerate(deviceprefix):
    groupdf = df[df['device_name'].str[0] == gpx]
    # groupdf = groupdf.pivot_table(index=['device_name','date_created_utc'],values=['smt_vwc_raw'])
    # groupdf = groupdf.pivot_table(index=['device_name', 'date_created_utc'], values=['chmln_top_ohms', 'chmln_bot_ohms'])
    # groupdf = groupdf.pivot_table(index=['device_name','date_created_utc'],values=['chmln_bot_ohms'])
    # groupdfs = groupdf.unstack(level=0)

    # ax = axs[idx]

    for cidx, c in enumerate(groupdf['device_name'].unique()):
        g = groupdf[groupdf['device_name'] == c]
        # Convert ohms to cb, which is the equivalent of the watermark reading
        g['chmln_top_cb'] = apply_ohm_to_cb(g, chameleonKey='chmln_top_ohms', tempKey='ds18b20_top_temp_c')
        g['chmln_bot_cb'] = apply_ohm_to_cb(g, chameleonKey='chmln_bot_ohms', tempKey='ds18b20_bot_temp_c')
        g['smt_vwc_pct'] = convert_smt_vwc(g)
        g['smt_temp_c'] = convert_smt_temp(g)
        # g = g[g['uptime_s'] > 50000]
        # ax = g.plot(y=['chmln_top_cb', 'chmln_bot_cb','ds18b20_top_temp_c', 'ds18b20_bot_temp_c','smt_vwc_pct','smt_temp_c'])
        ax = g.plot(y=['chmln_top_cb', 'chmln_bot_cb'])
        ax.set_title('Device: {}'.format(c))
        #ax.plot(rawbat.index, rawbat, label=c[0], color=colors[cidx])
        # # Fill in gaps linearly, showing the raw data as a thin dotted line
        # linbat = pd.Series(rawbat).interpolate(
        #     method='time', limit_direction='forward', limit_area='inside')
        # # Derive the rolling median from the battery data, plotting it as a thicker line with the same colour
        # avgbat = pd.Series(rawbat).rolling('1h').mean()
        # # _linecolor = colors[cidx % len(colors)]
        # linbat.plot(,sharex=True,lw=1,ls='--')
        # # avgbat.plot(ax=ax, sharex=True, style=[_linecolor], lw=1, ls='-')
        # ax.autoscale(True, axis='y', tight=True)
        # ax.legend()
        # ax.set_ylim(0, 6000)

    # plt.title(gpx)
    # plt.xlabel('Time')
    # plt.legend()

plt.tight_layout()
plt.show()
