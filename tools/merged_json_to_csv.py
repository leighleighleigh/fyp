#!/usr/bin/env python3

"""
--- Leigh's BOSL DATA CONVERTER TOOL ---

DESCRIPTION
Takes a merged.json file, containing a JSON message on each line, and converts it to a CSV file with raw + calculated fields.

This tool expects the 'raw' data format, but can optionally include the 'legacy' format as well - minus the 'raw data' fields.
    (the legacy format contains pre-calcualted data, generated on the BoSL board itself, so it may yield different results)

AUTHOR
Leigh Oliver, loli0003@student.monash.edu, leighcharlesoliver@gmail.com
Last updated 10/2022

USAGE
python3 merged_json_to_csv.py <input_file> <output_file> [--legacy] [--start <start_time>] [--end <end_time>]
"""

import datetime
import json
import typing
from pathlib import Path
from time import time_ns
import time

import pandas as pd
import pytz
from mcap.well_known import MessageEncoding, SchemaEncoding
from mcap.writer import Writer
from numpy import polyfit
from tqdm import tqdm

# Argument parsing for date range trim
import argparse
parser = argparse.ArgumentParser(description='Converts line-separated JSON message files into CSV files.')
parser.add_argument('input', type=str, help='Input filename of a line-separated JSON file', default="merged.json")
parser.add_argument('output', type=str, help='Output CSV filename', default="merged.csv", nargs='?')
parser.add_argument('--legacy', action='store_true', help='Use legacy data format (pre-calculated values)', default=False)
parser.add_argument('--start', type=str, help='Start date to trim to (inclusive). e.g: "2022-10-21 22:00:00"', default=None)
parser.add_argument('--end', type=str, help='End date to trim to (inclusive). e.g: "2022-10-23 22:00:00"', default=None)
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

    # Provide progress bar from 1/3, of each groups (C,P,S)
    # tqdm bar for device prefix enumeration
    with tqdm(deviceprefix, desc="Group", unit="group") as gbar:
        for idx, gpx in enumerate(deviceprefix):
            # Set progress bar description
            gbar.set_description(f"Group {gpx}")

            # Get a view of df, where id==gpx
            device_group_mask = df['id'].str.startswith(gpx)
            device_group = df.loc[device_group_mask]

            # All calculated data for this group is stored here, for averaging later
            # Blank dataframe same as device_group
            device_group_data = []

            # Iterate over individual devices, e.g C1/C2/C3 in group C.
            with tqdm(device_group['id'].unique(), desc="Device", position=1, leave=False, unit="device") as dbar:
                for cidx, c in enumerate(device_group['id'].unique()):
                    # Update bar description
                    dbar.set_description(f"Device {c}")

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

                    # Handle outliers in chameleon data - such as a single channel 'stuck' at 1023
                    # A CHA/CHB value of ~900 corresponds to a chameleon resistance of 1.36kOhms, which is VERY wet!
                    # There is a high chance of additional noise in this sensing region, due to the low impedence of the sensor,
                    # so we will clamp any values above a set threshold.
                    chameleonOutlierThreshold = 900

                    for i, row in device.iterrows():

                        # If either one of the values is above the threshold, we will set the outlier to the clamped minimum of the two channels.
                        if row['uCHA'] > chameleonOutlierThreshold or row['uCHB'] > chameleonOutlierThreshold:
                            device.loc[i, 'uCHA'] = min([chameleonOutlierThreshold, row['uCHA'], row['uCHB']])
                            device.loc[i, 'uCHB'] = min([chameleonOutlierThreshold, row['uCHA'], row['uCHB']])
                        
                    # Calculate average of Chameleon A/B channels, to derive resistance
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
                    # swapTempList = ["C2","P5","P4","C3","C4","S2","S4","S5"]

                    # Temperature swaps validated on 23/10/2022
                    swapTempList = ["C2","C3","C4","C5","P4","P5","S1","S2","S4","S5"]

                    if c in swapTempList:
                        swapcolumns("uT","lT")

                    # Swap chameleons if they were wired incorrectly
                    # NO SWAPS validated on 23/10/2022.
                    # Potential for miswired chameleons still possible.
                    swapChameleonList = []
                    # swapChameleonList = []

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


                    # Remove zeros from the data, which occur due to noise on the analog readings
                    rmzero('smtVWC_pct')
                    rmzero('smtT_c')
                    rmzero('uCB')
                    rmzero('lCB')
                    rmzero('uCB_nocal')
                    rmzero('lCB_nocal')

                    # Calculate normalised series of uCB and lCB, uCB_norm and lCB_norm
                    device.loc[:,'uCB_norm'] = (device['uCB'] - device['uCB'].mean()) / abs(device['uCB'].std())
                    device.loc[:,'lCB_norm'] = (device['lCB'] - device['lCB'].mean()) / abs(device['lCB'].std())

                    # Outlier columns list - removed from group averages 
                    # outlier_columns = ["C4","C5","P4","S4","S2"]
                    outlier_columns = ["C4","S4","P4"]

                    # Write out the data to mcap file, using another progress bar!
                    with tqdm(total=len(device), desc="Writing to MCAP file", unit="rows", leave=False) as pbar:
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

                            pbar.update(1)

                    dbar.update(1)


            # Create group average channel and publish the mean, resampled group data here
            channel_calc_group_avg_id = writer.register_channel(
                topic=f"/group_{gpx}_avg",
                message_encoding=MessageEncoding.JSON,
                schema_id=schema_calc,
            )

            # Resample device_group_data and average it
            device_group_data = pd.DataFrame(device_group_data)

            # Interpolate data to a 1 hour frequency
            device_group_data = device_group_data.resample('5min').median()

            # 5 minute rolling mean of device_group_data
            device_group_data = device_group_data.rolling('6H',center=False).mean()
            device_group_data = device_group_data.resample('1H').mean()

            # Write out the data to mcap file
            for i, row in device_group_data.iterrows():
                t = int(row.name.to_pydatetime().timestamp()*1e9)

                writer.add_message(
                    channel_calc_group_avg_id,
                    log_time=t,
                    data=row.to_json().encode('utf-8'),
                    publish_time=t
                )

            gbar.update(1)

    writer.finish()

