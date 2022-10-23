#!/usr/bin/python3

# Using pandas, loads a CSV file with headers 'column_id','start_time','end_time',
# and calculates the difference between start and end times in seconds.
# An asterisk in the column_id field indicates a malfunction during measurement, such as a leaky column.
import pandas as pd
import numpy as np
import argparse
import prettytable
from prettytable import MARKDOWN

# Loads an input filename from the command line
if __name__ == "__main__":
    # Get input filename
    parser = argparse.ArgumentParser()
    parser.add_argument("input_filename", help="The input filename")
    # The default head height is 100mm, but we can pass another value here if we want
    parser.add_argument("--head_height", help="The full water height in mm", type=int, default=100)
    args = parser.parse_args()

    # Load the CSV file, and convert to datetimes
    df = pd.read_csv(args.input_filename)

    # Convert the time columns to datetime objects
    df['start_time'] = pd.to_datetime(df['start_time'])
    df['end_time'] = pd.to_datetime(df['end_time'])

    # If end time is before start time, add one day
    df.loc[df['end_time'] < df['start_time'], 'end_time'] += pd.Timedelta(days=1)

    # Calculate the difference between start and end times, in seconds
    df['time_diff'] = df['end_time'] - df['start_time']
    df['time_diff'] = df['time_diff'].dt.total_seconds()

    # Calculate the infiltration rate in mm/hr ! 
    df['infiltration_rate'] = args.head_height / df['time_diff'] * 3600

    # Done :) Print out as a nice prettytable
    pt = prettytable.PrettyTable()
    # Verbose table with all data
    # pt.field_names = ["column_id", "start_time", "end_time", "time_diff", "infiltration_rate"]
    # Summary table
    pt.field_names = ["column_id", "infiltration_rate"]
    pt.float_format = ".1"

    for index, row in df.iterrows():
        pt.add_row([row[x] for x in pt.field_names])

    pt.sortby = "infiltration_rate"
    # Print as markdown table
    pt.set_style(MARKDOWN)
    print(pt)
    print("* malfunction of column observed")
