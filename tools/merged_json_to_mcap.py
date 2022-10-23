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

# Argument parsing for date range trim
import argparse
parser = argparse.ArgumentParser(description='Converts line-separated JSON message files into MCAP files.')
parser.add_argument('input', type=str, help='Input filename of a line-separated JSON file', default="merged.json")
parser.add_argument('output', type=str, help='Output MCAP filename', default="merged.mcap")
parser.add_argument('--start', type=str, help='Start date to trim to (inclusive). e.g: 2022-10-21 22:00:00', default=None)
parser.add_argument('--end', type=str, help='End date to trim to (inclusive). e.g: 2022-10-23 22:00:00', default=None)
args = parser.parse_args()


# Creates a Pandas Dataframe object from the json data
df = pd.read_json(args.input, orient="records", lines=True)
# Replace index with the converted date_created_utc column
df['date_created_utc'] = pd.to_datetime(df['date_created_utc'])
df = df.set_index('date_created_utc')
# Convert index to australian timezone (from UTC)
eastern = pytz.timezone('Australia/Melbourne')
df.index = df.index.tz_localize(pytz.utc)
df.index = df.index.tz_convert(eastern)

# If --start or --end times were provided, trim the dataframe to that range.
# These arguments must be parsed from string to datetime objects
if args.start is not None:
    start = datetime.datetime.strptime(args.start, '%Y-%m-%d %H:%M:%S')
    # Make the datetime timezone-aware
    start = pytz.timezone('Australia/Melbourne').localize(start)
    # Apply start time to dataframe index, avoiding key errors
    df = df[df.index >= start]


if args.end is not None:
    end = datetime.datetime.strptime(args.end, '%Y-%m-%d %H:%M:%S')
    end = pytz.timezone('Australia/Melbourne').localize(end)
    df = df[df.index <= end]


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


# Group these names based on prefix, which is the first letter
deviceprefix = ['C', 'P', 'S']

# Open mcap file
with open(args.output,"wb") as f_mcap:
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
        topic=f"/raw",
        message_encoding=MessageEncoding.JSON,
        schema_id=schema_raw,
    )

    column_channel_calc = writer.register_channel(
        topic="/calc",
        message_encoding=MessageEncoding.JSON,
        schema_id=schema_calc,
    )

    for idx, gpx in enumerate(deviceprefix):
        # Get a view of df, where id==gpx
        device_group_mask = df['id'].str.startswith(gpx)
        device_group = df.loc[device_group_mask]

        # All calculated data for this group is stored here, for averaging later
        # Blank dataframe same as device_group
        device_group_data = []

        # Iterate over individual devices, e.g C1/C2/C3 in group C.
        for cidx, c in enumerate(device_group['id'].unique()):
            # Get the single device from the group, using another DF mask
            device_mask = df['id'].str.contains(c)
            device = df.loc[device_mask].copy()

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
            device.loc[:,'uCHAB'] = ((device['uCHA'] + device['uCHB'])/2)
            # Explicit notation for resistance conversion
            # device['uCHR'] = 10 * (1023.0 - device['uCHAB']) / device['uCHAB']
            # Alternate conversion using map function
            device.loc[:,'uCHR'] = device['uCHAB'].map(lambda x: 10 * (1023.0 - x) / x)
            # Lower chameleon
            device.loc[:,'lCHAB'] = ((device['lCHA'] + device['lCHB'])/2)
            # device['lCHR'] = 10 * (1023.0 - device['lCHAB']) / device['lCHAB']
            device.loc[:,'lCHR'] = device['lCHAB'].map(lambda x: 10 * (1023.0 - x) / x)


            def swapcolumns(a : str,b : str):
                device.loc[:,a+"_"] = device.loc[:,a]
                device.loc[:,b+"_"] = device.loc[:,b]
                device.loc[:,a] = device.loc[:,b+"_"]
                device.loc[:,b] = device.loc[:,a+"_"]

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
            # Convert from resistance of chameleon sensor, to a water tension reading in centi-bar.
            # Temperature is also used to apply the known temperature calibration model.
            device.loc[:,'uCB'] = apply_kohm_to_cb(device, chameleonKey='uCHR', tempKey='uT')
            device.loc[:,'lCB'] = apply_kohm_to_cb(device, chameleonKey='lCHR', tempKey='lT')

            device.loc[:,'uCB_nocal'] = apply_kohm_to_cb(device, chameleonKey='uCHR')
            device.loc[:,'lCB_nocal'] = apply_kohm_to_cb(device, chameleonKey='lCHR')
            
            # Convert to percentages
            device.loc[:,'smtVWC_pct'] = convert_smt_vwc(device)
            device.loc[:,'smtT_c'] = convert_smt_temp(device)

            # Remove zero-values
            def rmzero(key):
                # Remove rows where the column 'key' are lessthan or equal to zero
                device.drop(device[device[key] <= 0].index, inplace=True)
                # device[key] = device[key][device[key]>0].copy()


            # Remove zeros from the data, which occur due to noise on the analog readings
            rmzero('smtVWC_pct')
            rmzero('smtT_c')
            rmzero('uCB')
            rmzero('lCB')
            rmzero('uCB_nocal')
            rmzero('lCB_nocal')


            # Smooth the data into 10 minute rolling averages
            smoothperiod = '1h'

            def smoothinline(key):
                # Smooth only the column 'key' using a rolling average
                return device[key].rolling(smoothperiod,center=True).mean()

            def smooth(key):
                # Apply rolling average to the 'key' column, using '.loc'
                device.loc[:,key] = smoothinline(key)


            smooth('smtVWC_pct')
            smooth('smtT_c')
            smooth('uCB')
            smooth('lCB')
            smooth('uCB_nocal')
            smooth('lCB_nocal')
            smooth('uT')
            smooth('lT')


            # Do funky polyfit window thingy to derive the rate of change
            def rollinglintrend(key):
                # each sample is ~5 minutes
                # 12 samples is 1 hour
                # 6 samples is 30 minutes
                # Apply a rolling linear trend to the 'key' column, using '.loc'
                return device[key].rolling(6).apply(lambda x: polyfit(range(len(x)), x, 1)[0])

            device.loc[:,'uCB_rate'] = rollinglintrend("uCB")
            device.loc[:,'lCB_rate'] = rollinglintrend("lCB")

            # Calculate the rate of change of chmln_top_cb, w.r.t the time index
            # device['uCB_rate'] = device['uCB'].diff() / device['uCB'].index.to_series().diff().dt.total_seconds()
            # device['lCB_rate'] = device['lCB'].diff() / device['lCB'].index.to_series().diff().dt.total_seconds()

            # Convert the rate from cb/s to cb/h
            # device['uCB_rate'] = device['uCB_rate'] * 3600
            # device['lCB_rate'] = device['lCB_rate'] * 3600

            # Smooth!
            smooth('uCB_rate')
            smooth('lCB_rate')

            # Outlier columns list - removed from group averages 
            outlier_columns = ["P4","C4"]

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
                    column_channel_calc,
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

                if c not in outlier_columns:
                    device_group_data.append(row.copy())

                writer.add_message(
                    column_channel,
                    log_time=t,
                    data=row.to_json().encode('utf-8'),
                    publish_time=t
                )

        # Create group average channel and publish the mean, resampled group data here
        channel_calc_group_avg_id = writer.register_channel(
            topic=f"/group_{gpx}_avg",
            message_encoding=MessageEncoding.JSON,
            schema_id=schema_calc,
        )

        # Resample device_group_data and average it
        device_group_data = pd.DataFrame(device_group_data)
        # Interpolate data to a 1 hour frequency
        device_group_data = device_group_data.resample('1H').median().interpolate()

        # Write out the data to mcap file
        for i, row in device_group_data.iterrows():
            t = int(row.name.to_pydatetime().timestamp()*1e9)

            writer.add_message(
                channel_calc_group_avg_id,
                log_time=t,
                data=row.to_json().encode('utf-8'),
                publish_time=t
            )

    writer.finish()

