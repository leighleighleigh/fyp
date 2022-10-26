#!/usr/bin/env python3

"""
Loads merged.json, and produces 'imei_db.json'. 
This file is used to map legacy data cell phone numbers into IMEI numbers,
and also to store the relation from ID/IMEI/phone number.
"""

import json
import pandas as pd

# Load the merged.json file
df = pd.read_json("merged.json", orient="records", lines=True)

# Create a new dataframe with just the IMEI and cellnumber
df_imei = df[['id', 'imei']]
df_cellnum = df[['device_name', 'cellnumber']]

# Get all rows in df, with unique IMEI, and store as imei df
df_imei = df_imei.drop_duplicates(subset=['imei'])
# Repeat for cellnumbers
df_cellnum = df_cellnum.drop_duplicates(subset=['cellnumber'])
# Rename 'device_name' to 'id'
df_cellnum = df_cellnum.rename(columns={'device_name': 'id'})
# Set index of both frames to the 'id', so we can merge them
df_imei = df_imei.set_index('id')
df_cellnum = df_cellnum.set_index('id')

# Add the 'cellnumber' column into the imei df, by matching id
df_imei['cellnumber'] = df_cellnum['cellnumber']
# Remove nAn values
df_imei = df_imei.dropna()

# change imei and cellnumber to integers, then strings 
df_imei['imei'] = df_imei['imei'].astype(int)
df_imei['cellnumber'] = df_imei['cellnumber'].astype(int)
df_imei['imei'] = df_imei['imei'].astype(str)
df_imei['cellnumber'] = df_imei['cellnumber'].astype(str)
# Prefix cellnumber with a '0'
df_imei['cellnumber'] = '0' + df_imei['cellnumber']

# Print!
print(df_imei)

# Save as imei_db.json, to read from in scripts
df_imei.to_json("imei_db.json", orient="index")
# Save also as a csv file, for easy viewing
df_imei.to_csv("imei_db.csv")