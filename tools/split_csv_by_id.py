#!/usr/bin/env python3

"""
DESCRIPTION
Takes a CSV file containing the 'id' column, and produces individual CSV files for each unique ID.

AUTHOR
Leigh Oliver, loli0003@student.monash.edu, leighcharlesoliver@gmail.com
Last updated 10/2022

USAGE
python3 split_csv_by_id.py <input_file> 
"""

from pathlib import Path
from data_conversions import *
import datetime
import json
import pandas as pd
import pytz
import time
import typing
import argparse

parser = argparse.ArgumentParser(description='Splits a CSV file into multiple files by unique values in the "id" column')
parser.add_argument('input', type=str, help='Input CSV file', default="merged.csv", nargs='?')
args = parser.parse_args()

# Load CSV as pandas
df = pd.read_csv(args.input)

# Split df into groups by unique values of 'id' column
groups = df.groupby('id')

# Write each group as a CSV file to the 'data_split' directory
for name, group in groups:
    group.to_csv(f"data_split/{name}.csv", index=False)

# Now we will generate a CSV for each GROUP, which is the first letter of the id field
groups = df.groupby(df['id'].str[0])
# This df will contain all three groups
alldf = None

for name, group in groups:
    # Make blank dataframe
    df = None

    # Group individual columns within the parent group
    columngroup = group.groupby('id')

    for colname, colgroup in columngroup:
        # Set index to date time
        colgroup.set_index('date_created_utc', inplace=True)
        # Prefix the name onto the group columns
        colgroup.columns = [f"{colname}_{col}" for col in colgroup.columns]

        # If df is none, set it to this first colgroup
        if df is None:
            df = colgroup
        else:
            df = df.join(colgroup, how='outer')

        # If alldf is none, set it to this first colgroup
        if alldf is None:
            alldf = colgroup
        else:
            alldf = alldf.join(colgroup, how='outer')

    df.to_csv(f"data_split/group_{name}.csv", index=True)

alldf.to_csv(f"data_split/all_columns.csv", index=True)



