#!/usr/bin/env python3

"""
DESCRIPTION
Takes a CSV file containing the 'id' column, and produces individual CSV files for each unique ID.
Additionallly, it combines columns into a single CSV format, with prefixed column names. 
Groups of columns (C,P,S) are combined into a similar format - with additional group_average columns.


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

# Ensure that date_created_utc is appropriately set as the index
df['date_created_utc'] = pd.to_datetime(df['date_created_utc'])
df = df.set_index('date_created_utc')


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
        # colgroup.set_index('date_created_utc', inplace=True)
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
    
    # df now contains all the individual devices witin a single group,
    # with the device ID prefixed onto the CSV column names.
    # We will not calculate the group medians, mean, and standard deviation.

    # This function will take a column suffix, and return all matching series in df
    def get_series_by_suffix(df, suffix):
        return df.loc[:, df.columns.str.endswith(suffix)]
    
    # We can now obtain the group medians, by first getting all series of the desired suffix.
    # First, we will calculate the median of smt_vwc_percentage
    median_smt_vwc_percentage = get_series_by_suffix(df, 'smt_vwc_percentage')

    # Iterate over a rolling datetime window, taking the mean of all columns, and storing the result into a new column
    for i in median_smt_vwc_percentage.iterrows():
        # i[0] is the datetime index
        # i[1] is the series of values
        df.loc[i[0], f'group_{name}_median_smt_vwc_percentage'] = i[1].median() 
        df.loc[i[0], f'group_{name}_mean_smt_vwc_percentage'] = i[1].mean() 
        df.loc[i[0], f'group_{name}_stddev_smt_vwc_percentage'] = i[1].std() 

    # Add this series to the dataframe
    # df[f'group_{name}_median_smt_vwc_percentage'] = median_smt_vwc_percentage

    #df.loc[:,f'group_{name}_median_smt_vwc_percentage'] = median_smt_vwc_percentage.rolling('30min').median()
    #alldf.loc[:,f'group_{name}_median_smt_vwc_percentage'] = median_smt_vwc_percentage.rolling('30min').median()

    # Finally, write out the single gropu to CSV file.
    df.to_csv(f"data_split/group_{name}.csv", index=True)

# This is the same as above, but for devices in a single file.
alldf.to_csv(f"data_split/all_columns.csv", index=True)



