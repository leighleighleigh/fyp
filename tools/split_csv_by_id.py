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


