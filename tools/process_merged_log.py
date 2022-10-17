#!/usr/bin/env python3

from mcap.mcap0.well_known import SchemaEncoding, MessageEncoding
from mcap.mcap0.writer import Writer
from pathlib import Path
from time import time_ns
import argparse
import base64
import csv
import datetime
import json
import matplotlib
import matplotlib as mpl
import matplotlib.pyplot as plt
import pandas as pd
import pytz
import struct
import typing
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

# Open mcap file
with open("merged.mcap","wb") as f_mcap:
    # Build an MCAP writer for this device
    writer = Writer(f_mcap)
    writer.start("x-jsonschema", library="bosl-json")

    # Read schema 
    with open(Path(__file__).parent / "ColumnDataRaw.json", "rb") as f:
        schema = f.read()

    # Build writers for this schema
    schema_id = writer.register_schema(
        name="bosl.ColumnDataRaw",
        encoding=SchemaEncoding.JSONSchema,
        data=schema,
    )

    for idx, gpx in enumerate(deviceprefix):
        device_group = df[df['device_name'].str[0] == gpx]
        # Iterate over individual devices
        for cidx, c in enumerate(device_group['device_name'].unique()):
            # g contains the data for a single column within this column group
            device = device_group[device_group['device_name'] == c]

            # Extract all the device ID's in the source file
            channel_id = writer.register_channel(
                topic=f"{c}/raw",
                message_encoding=MessageEncoding.JSON,
                schema_id=schema_id,
            )

            # Write out the data to mcap file
            for i, row in device.iterrows():
                t = int(row.name.to_pydatetime().timestamp()*1e9)
                writer.add_message(
                    channel_id,
                    log_time=t,
                    data=row.to_json().encode('utf-8'),
                    publish_time=t
                )

            # Apply filters to the device data
            device['chmln_top_cb'] = apply_ohm_to_cb(
                device, chameleonKey='chmln_top_ohms', tempKey='ds18b20_top_temp_c')
            device['chmln_bot_cb'] = apply_ohm_to_cb(
                device, chameleonKey='chmln_bot_ohms', tempKey='ds18b20_bot_temp_c')
            device['smt_vwc_pct'] = convert_smt_vwc(device)
            device['smt_temp_c'] = convert_smt_temp(device)
            device['chmln_top_cb_rolling'] = pd.Series(
                device['chmln_top_cb']).rolling('10min').median()
            device['chmln_bot_cb_rolling'] = pd.Series(
                device['chmln_bot_cb']).rolling('10min').median()

            # # Plot the data
            # ax = device.plot(y=['chmln_top_cb', 'chmln_bot_cb', 'chmln_top_cb_rolling', 'chmln_bot_cb_rolling',
            #             'ds18b20_top_temp_c', 'ds18b20_bot_temp_c', 'smt_vwc_pct', 'smt_temp_c'])

            # ax.set_title('Device: {}'.format(c))

    writer.finish()

# plt.tight_layout()
# plt.show()
