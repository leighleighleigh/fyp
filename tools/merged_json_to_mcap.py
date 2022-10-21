#!/usr/bin/env python3

import datetime
import json
import typing
from pathlib import Path
from time import time_ns

import pandas as pd
import pytz
from mcap.well_known import MessageEncoding, SchemaEncoding
from mcap.writer import Writer
from numpy import polyfit

# Creates a Pandas Dataframe object from the json data
df = pd.read_json("merged.json", orient="records", lines=True)


# def unpack(x):
#     rv = []
#     for v in x:
#         if isinstance(v, dict):
#             rv.append([*v.values()][0])
#         else:
#             rv.append(v)
#     return rv


# df = df.apply(unpack)
#print(df)

# Replace index with the converted date_created_utc column
df['date_created_utc'] = pd.to_datetime(df['date_created_utc'])
df = df.set_index('date_created_utc')
#print(df)
# Convert index to australian timezone (from UTC)
eastern = pytz.timezone('Australia/Melbourne')
df.index = df.index.tz_localize(pytz.utc)
#print(df)
df.index = df.index.tz_convert(eastern)
#print(df)

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
    schema_raw = writer.register_schema(
        name="bosl.ColumnDataRaw",
        encoding=SchemaEncoding.JSONSchema,
        data=schema,
    )

    with open(Path(__file__).parent / "ColumnData.json", "rb") as f:
        schema = f.read()

    schema_calc = writer.register_schema(
        name="bosl.ColumnData",
        encoding=SchemaEncoding.JSONSchema,
        data=schema,
    )

    column_channel = writer.register_channel(
        topic=f"/raw_columns",
        message_encoding=MessageEncoding.JSON,
        schema_id=schema_raw,
    )

    for idx, gpx in enumerate(deviceprefix):
        device_group = df[df['id'].str[0] == gpx]

        # Iterate over individual devices
        for cidx, c in enumerate(device_group['id'].unique()):
            # g contains the data for a single column within this column group
            device = device_group[device_group['id'] == c]

            # Extract all the device ID's in the source file
            channel_raw_id = writer.register_channel(
                topic=f"{c}/raw",
                message_encoding=MessageEncoding.JSON,
                schema_id=schema_raw,
            )

            channel_calc_id = writer.register_channel(
                topic=f"{c}/calc",
                message_encoding=MessageEncoding.JSON,
                schema_id=schema_calc,
            )

            channel_calc_group_id = writer.register_channel(
                topic=f"/group_{c[0]}",
                message_encoding=MessageEncoding.JSON,
                schema_id=schema_calc,
            )

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

            # Smooth the data into 10 minute rolling averages
            smoothperiod = '10min'
            # 30 minutes is a little slow
            #smoothperiod = '30min'


            def smoothinline(key):
                return pd.Series(device[key]).rolling(smoothperiod,center=True).mean().copy()

            def smooth(key):
                device[key] = smoothinline(key)

            # rmzero('smt_vwc_pct')
            # rmzero('smt_temp_c')
            # rmzero('chmln_top_ohms')
            # rmzero('chmln_bot_ohms')
            # rmzero('chmln_top_cb')
            # rmzero('chmln_bot_cb')
            # rmzero('chmln_top_cb_uncalibrated')
            # rmzero('chmln_bot_cb_uncalibrated')
            # rmzero('ds18b20_top_temp_c')
            # rmzero('ds18b20_bot_temp_c')

            # smooth('smt_vwc_pct')
            # smooth('smt_temp_c')

            # smooth('chmln_top_cb')
            # smooth('chmln_bot_cb')
            # smooth('chmln_top_cb_uncalibrated')
            # smooth('chmln_bot_cb_uncalibrated')
            # smooth('ds18b20_top_temp_c')
            # smooth('ds18b20_bot_temp_c')

            # Create subtraction entry
            # device['chmln_top_cb_rate'] = device['chmln_top_cb'].diff()
            # device['chmln_bot_cb_rate'] = device['chmln_bot_cb'].diff()

            # Do funky polyfit window thingy
            def rollinglintrend(key):
                # each sample is ~2 minutes
                # 30 samples is 1 hour
                # 5 samples is 10 minutes
                return pd.Series(device[key].copy()).rolling(5).apply(lambda x: polyfit(range(len(x)), x, 1)[0])

            # device['chmln_top_cb_rate'] = rollinglintrend("chmln_top_cb")
            # device['chmln_bot_cb_rate'] = rollinglintrend("chmln_bot_cb")

            # Calculate the rate of change of chmln_top_cb, w.r.t the time index
            # device['chmln_top_cb_rate'] = device['chmln_top_cb'].diff() / device['chmln_top_cb'].index.to_series().diff().dt.total_seconds()
            # device['chmln_bot_cb_rate'] = device['chmln_bot_cb'].diff() / device['chmln_bot_cb'].index.to_series().diff().dt.total_seconds()

            # Convert the rate from cb/s to cb/h
            # device['chmln_top_cb_rate'] = device['chmln_top_cb_rate'] * 3600
            # device['chmln_bot_cb_rate'] = device['chmln_bot_cb_rate'] * 3600

            # Write out the data to mcap file
            for i, row in device.iterrows():
                t = int(row.name.to_pydatetime().timestamp()*1e9)

                # print(row.to_json().encode('utf-8')

                # Raw data message
                writer.add_message(
                    channel_raw_id,
                    log_time=t,
                    data=row.to_json().encode('utf-8'),
                    publish_time=t
                )

                writer.add_message(
                    channel_calc_id,
                    log_time=t,
                    data=row.to_json().encode('utf-8'),
                    publish_time=t
                )

                writer.add_message(
                    channel_calc_group_id,
                    log_time=t,
                    data=row.to_json().encode('utf-8'),
                    publish_time=t
                )

                writer.add_message(
                    column_channel,
                    log_time=t,
                    data=row.to_json().encode('utf-8'),
                    publish_time=t
                )

    writer.finish()
