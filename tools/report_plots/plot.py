#!/usr/bin/env python3
from pathlib import Path, PurePath
from pyfzf.pyfzf import FzfPrompt
from termcolor import colored
import click
import json
import matplotlib.pyplot as plt
import os
import numpy as np
from typing import Any, List
import sys

"""
bosl log plotter
- select a file with JSON line encoding for data entries
- select which json key to be the x-axis
- select which json key to be the y-axis
- plot!
"""

"""
Custom exceptions
"""


class BoSLPlotException(Exception):
    ...


class NoFileProvidedException(BoSLPlotException):
    def __init__(self):
        super().__init__(
            colored("Please provide a BoSL log file.", "red", attrs=["bold"]))


"""
SELECT INPUT FILE
"""


def prompt_for_file() -> Path:
    # Get .csv files in the current working directory
    cwd = Path.cwd()
    cwdfiles = map(lambda x: x.name, cwd.glob("*.csv"))

    # Prompt to select file form this list
    print(colored("Select a BOSL datalog file", 'white', attrs=['bold']))

    # Select with fzf, the best fuzzy finder :)
    fzf = FzfPrompt()
    fzfopts = """--height 40% --layout reverse"""
    result = fzf.prompt(list(cwdfiles), fzfopts)

    # Check for a result
    if len(result) == 1:
        filepath = cwd / str(result[0])
        return filepath.resolve()
    else:
        raise NoFileProvidedException()


def load_file_keys(logfile: Path) -> List[PurePath]:
    print(colored(f"Loading {logfile.name}...", "white", attrs=["bold"]))
    with open(logfile) as data:
        # Read one JSON entry to acquire all keys we will be plotting against
        while entry := data.readline():
            if len(entry) > 3:
                break

        # Parse json
        entrydata = json.loads(entry)
        entrykeypaths = []

        def extract_key_paths(json, parents):
            for k, v in json.items():
                if isinstance(v, dict):
                    extract_key_paths(v, f"{parents}/{k}")
                else:
                    pathstr = f"{parents}/{k}".lstrip(' ')
                    entrykeypaths.append(PurePath(pathstr))

        extract_key_paths(entrydata, "")

        return entrykeypaths

def load_file_data(logfile : Path, keypath : PurePath) -> List[Any]:
    # Make a list to contain the data
    data = []
    # Open the file
    with open(logfile) as f:
        # Read  lines, extract key item, add to list
        while linejson := f.readline():
            # Discard empty lines
            if len(linejson) <= 3:
                continue
            else:
                # Parse line JSON
                linedata = json.loads(linejson)
                # Use the keypath to traverse the json
                for p in keypath.parts:
                    if p == "/":
                        continue
                    linedata = linedata[p]

                # Should be at our data now
                data.append(linedata)


    return data



@click.command()
@click.option("--logfile", default=None, help="JSON encoded file with .CSV extension.")
def run(logfile):
    if logfile is None:
        logfile = prompt_for_file()
    else:
        if isinstance(logfile, str):
            # Try resolve to path
            filename = Path(logfile)
            if filename.is_file():
                logfile = filename.resolve()
            else:
                raise NoFileProvidedException()

    key_paths = load_file_keys(logfile)
    print(colored("Select a key for x-axis", 'white', attrs=['bold']))
    fzf = FzfPrompt()
    fzfopts = """--height 40% --layout reverse"""
    x_axis_key = PurePath(fzf.prompt(list(key_paths), fzfopts)[0])
    # Remove key
    key_paths.remove(x_axis_key)
    x_axis_key_name = str(x_axis_key).lstrip("/")
    print(x_axis_key_name)
    x_data = load_file_data(logfile, x_axis_key)

    while(1):
        print(colored("Select a key for y-axis", 'white', attrs=['bold']))
        fzf = FzfPrompt()
        fzfopts = """--height 40% --layout reverse"""
        y_axis_key = PurePath(fzf.prompt(list(key_paths), fzfopts)[0])
        key_paths.remove(y_axis_key)
        y_axis_key_name = str(y_axis_key).lstrip("/")
        print(y_axis_key_name)

        # Get the data list
        y_data = load_file_data(logfile, y_axis_key)
        #y_data = list(map(lambda y: np.nan if y < 10 else y, y_data))
        y_data_norm = np.array(y_data)
        y_data = (y_data_norm - np.min(y_data_norm)) / (np.max(y_data_norm) - np.min(y_data_norm))
        #y_linalgsum = np.linalg.norm(y_data_norm)
        # y_linalgsum = np.ndarray.sum(y_data_norm,axis=0)
        # y_data = y_data_norm / y_linalgsum

        # Plot the data!
        with plt.style.context("seaborn-poster"):
            plt.plot(x_data,y_data,label=f"{y_axis_key_name}")
            plt.xlabel(x_axis_key_name)
            plt.ylabel("value")

        # Ask if we want to add another column
        q = input("Add another series? (y/n): ").lower()
        if "n" in q or "no" in q:
            plottitle = input("Enter plot title: ")
            plt.title(plottitle)
            break
    
    with plt.style.context("seaborn-poster"):
        plt.legend()
        plt.show()

run()
