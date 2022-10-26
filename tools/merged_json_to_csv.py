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
python3 merged_json_to_csv.py <input_file> <output_file> [--exclude-legacy] [--start <start_time>] [--end <end_time>]
"""

from pathlib import Path
from data_conversions import *
import datetime
import json
import pandas as pd
import pytz
import time
import typing

# Argument parsing for date range trim
import argparse
parser = argparse.ArgumentParser(description='Converts line-separated JSON message files into CSV files.')
parser.add_argument('input', type=str, help='Input filename of a line-separated JSON file', default="merged.json", nargs='?')
parser.add_argument('output', type=str, help='Output CSV filename', default="merged.csv", nargs='?')
parser.add_argument('--exclude-legacy', action='store_true', help='Exclude legacy data format (pre-calculated values) in the output', default=False)
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


# Returns True if the row is a 'legacy' row. This is indicated by the presence of the 'device_name' key.
def row_is_legacy(row : object):
    return row['device_name'] is not np.nan 

# Gets a key value, or returns np.nan if the key doesn't exist
# If legacykey is provided, the legacy value will be returned instead of NaN
def get_legacy_or_nan(row, key, legacykey=None):
    return row[key] if not row_is_legacy(row) else (np.nan if legacykey is None else row[legacykey])


# Conversion from chameleon readings to ohms
def get_row_chameleon_ohms(row, upper=True):
    if upper:
        return chameleon_raw_to_ohms(row["uCHA"],row["uCHB"]) if not row_is_legacy(row) else row["chmln_top_ohms"]
    else:
        return chameleon_raw_to_ohms(row["lCHA"],row["lCHB"]) if not row_is_legacy(row) else row["chmln_bot_ohms"]

# This dictionary maps OUTPUT column names to functions
# For each row in the input file, we iterate over the keys in this dictionary and call 'function(row)'
# The returned value is then used for the row's equivalent CSV entry
csvfuncs = {}
csvfuncs["date_created_utc"] = lambda row : row.name
csvfuncs["id"] = lambda row : row['id'] if not row_is_legacy(row) else row['device_name']
csvfuncs["legacy"] = lambda row : row_is_legacy(row) 
csvfuncs["battery_mv"] = lambda row : row['bat'] if not row_is_legacy(row) else row['batt_mv'] 

# Columns of raw data available from all devices (legacy and not)
csvfuncs["upper_chameleon_ohms"] = lambda row : get_row_chameleon_ohms(row, upper=True)
csvfuncs["lower_chameleon_ohms"] = lambda row : get_row_chameleon_ohms(row, upper=False)
csvfuncs["upper_temperature_celcius"] = lambda row : get_legacy_or_nan(row, "uT","ds18b20_top_temp_c")
csvfuncs["lower_temperature_celcius"] = lambda row : get_legacy_or_nan(row, "lT","ds18b20_bot_temp_c")
csvfuncs["smt_vwc_raw"] = lambda row : get_legacy_or_nan(row, "smtVWC","smt_vwc_raw")
csvfuncs["smt_temperature_raw"] = lambda row : get_legacy_or_nan(row, "smtT","smt_temp_raw")

# Calculate data from ohms to centi-bar, with temperature conversion.
csvfuncs["upper_chameleon_centibar"] = lambda row : ohm_to_cb(get_row_chameleon_ohms(row, upper=True), get_legacy_or_nan(row, "uT","ds18b20_top_temp_c"))
csvfuncs["lower_chameleon_centibar"] = lambda row : ohm_to_cb(get_row_chameleon_ohms(row, upper=False), get_legacy_or_nan(row, "lT","ds18b20_bot_temp_c"))
# Convert SMT values to SI units
csvfuncs["smt_vwc_percentage"] = lambda row : smt_raw_to_vwc(get_legacy_or_nan(row, "smtVWC","smt_vwc_raw"))
csvfuncs["smt_temperature_celcius"] = lambda row : smt_raw_to_temp(get_legacy_or_nan(row, "smtT","smt_temp_raw"))
# Non-legacy columns
csvfuncs["upper_chameleon_raw_channel_a"] = lambda row : get_legacy_or_nan(row, "uCHA")
csvfuncs["upper_chameleon_raw_channel_b"] = lambda row : get_legacy_or_nan(row, "uCHB")
csvfuncs["lower_chameleon_raw_channel_a"] = lambda row : get_legacy_or_nan(row, "lCHA")
csvfuncs["lower_chameleon_raw_channel_b"] = lambda row : get_legacy_or_nan(row, "lCHB")

# Open CSV file
with open(args.output,"w") as f_csv:
    # Write the keys of csvfuncs as the header
    f_csv.write(",".join(csvfuncs.keys()))

    # Iterate over each row in the dataframe, and write to the CSV file
    for index, row in df.iterrows():
        # If the row is a legacy row, and we don't want to include legacy rows, skip it
        if row_is_legacy(row) and args.exclude_legacy: 
            continue

        # Write the row to the CSV file
        f_csv.write("\n")
        f_csv.write(",".join([str(func(row)) for func in csvfuncs.values()]))
        f_csv.flush()
    