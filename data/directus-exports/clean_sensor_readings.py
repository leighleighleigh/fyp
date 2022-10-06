#!/usr/bin/env python3

# Loads a json file containing raw sensor data, in Directus JSON format.
# Saves a json file containing SI-valued data, in Directus JSON format.

import json
import click
import time
from numpy import interp
from pathlib import Path

def ohm_to_cb(rS : float, tempC : float = 24):
    """
    Converts a resistance reading of a Chameleon/Watermark sensor, and derives
    an approximate water tension reading in kPa.
    This also incorporates the temperature calibration of the sensor, although
    the default is 24C.
    """

    if rS < 550:
        return 0

    if rS < 1000:
        return abs((rS/1000*23.156-12.736)*-(1+0.018*(tempC-24)))

    if rS < 8000:
        return abs((-3.213*(rS/1000.00)-4.093)/(1-0.009733*(rS/1000.00)-0.01205*(tempC)))
    
    if rS < 35000:
        return abs(-2.246-5.239*(rS/1000.00)*(1+.018*(tempC-24.00))-.06756*(rS/1000.00)*(rS/1000.00)*((1.00+0.018*(tempC-24.00))*(1.00+0.018*(tempC-24.00))))

    # Open circuit!
    return 255

def smt_temp_conv(rawvalue : float):
    # Conversion is from SMT datasheet, but 10x scaled.
    # Regular SMT100 is 0-10V, ours it 0-1V.
    voltRange = 1.0
    adcMax = 1024
    adcVolt = 3.3
    rawVolt = (rawvalue/adcMax)*adcVolt
    return interp(rawVolt,[0,voltRange],[-40,60])

def smt_vwc_conv(rawvalue : float):
    # Conversion is from SMT datasheet, but 10x scaled.
    # Regular SMT100 is 0-10V, ours it 0-1V.
    voltRange = 1.0
    adcMax = 1024
    adcVolt = 3.3
    rawVolt = (rawvalue/adcMax)*adcVolt
    return interp(rawVolt,[0,voltRange],[0,100])


def clean_function(data : dict) -> dict:
    """
    Takes a single row-entry and converts it to a new one
    """
    # Remove id
    del data["id"]

    # Remove raw chameleon information
    del data["chmln_top_raw_a"]
    del data["chmln_top_raw_b"]
    del data["chmln_top_raw_avg"]
    del data["chmln_bot_raw_a"]
    del data["chmln_bot_raw_b"]
    del data["chmln_bot_raw_avg"]
    # Remove nonexistant SMT100 on the bottom
    del data["smt100_bot_temperature"]
    del data["smt100_bot_vwc"]

    # Convert the SMT100 readings into SI values.
    # The sensor used is a 1V output range.
    data["smt100_temperature"] = smt_temp_conv(data["smt100_top_temperature"])
    data["smt100_vwc"] = smt_vwc_conv(data["smt100_top_vwc"])
    del data["smt100_top_temperature"]
    del data["smt100_top_vwc"]

    # Store ds18b20
    data["ds18b20_top_temperature"] = data["smt100_temperature"]
    data["ds18b20_bot_temperature"] = data["smt100_temperature"]

    # Convert chameleon data
    # We will use the SMT for both sensors, storing the value into the dsb18b20 entries
    #data["chmln_top_kpa_uncalib"] = ohm_to_cb(data["chmln_top_resistance"])
    #data["chmln_bot_kpa_uncalib"] = ohm_to_cb(data["chmln_bot_resistance"])
    data["chmln_top_kpa"] = ohm_to_cb(data["chmln_top_resistance"],data["smt100_temperature"])
    data["chmln_bot_kpa"] = ohm_to_cb(data["chmln_bot_resistance"],data["smt100_temperature"])
    del data["chmln_top_resistance"]
    del data["chmln_bot_resistance"]

    if data["smt100_temperature"] == 60:
        return None

    return data

def clean_data(jsondata : dict):
    temp = list(map(clean_function, jsondata))
    tempnonnull = []
    for i in temp:
        if i is not None:
            tempnonnull.append(i)

    return tempnonnull

@click.command()
@click.option("--input-file", prompt="Input filename", help="The file containing raw data to be converted")
@click.option("--output-file", prompt="Output filename", help="")
def import_file(input_file : str, output_file : str):
    with open(input_file,'r') as f:
        filedata = json.load(f)
        cleandata = clean_data(filedata)
        with open(output_file, 'w') as fo:
            json.dump(cleandata,fo)
            #for l in cleandata:
                #if l["smt100_temperature"] != 60:
                    #json.dump(l,fo)
                    #fo.write("\n")


if __name__ == "__main__":
    import_file()
