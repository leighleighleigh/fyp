#!/usr/bin/env python3
"""
Converts line-separated JSON data from BoSL boards, into an MCAP file with 'ColumnDataRaw' messages.
Each device (column) is given it's own topic/channel, and the data is published in the prefix /<device_name>/raw
"""
from mcap.mcap0.well_known import SchemaEncoding, MessageEncoding
from mcap.mcap0.writer import Writer
from pathlib import Path
from time import time_ns
import argparse
import base64
import csv
import datetime
import json
import struct
import typing

def main():
    # read the merged json file
    with open("merged.json","r") as rf:
        data = json.load(rf)
        # Extract unique device names
        device_names = data
        # tutorial-write-header-start
        with open("merged.mcap", "wb") as f:
            writer = Writer(f)
            writer.start("x-jsonschema", library="bosl-json")

            # Read schema 
            with open(Path(__file__).parent / "ColumnDataRaw.json", "rb") as f:
                schema = f.read()

            # Build writers for this schema
            schema_id = writer.register_schema(
                name="bosl.ColumnDataRaw",
                encoding=SchemaEncoding.JSONSchema,
                data=schema,
            )

            # Extract all the device ID's in the source file
            channel_id = writer.register_channel(
                topic="",
                message_encoding=MessageEncoding.JSON,
                schema_id=schema_id,
            )

            for i in range(10):
                tomato: typing.Dict[str, typing.Any]
                tomato = {}
            
                tomato["mass"] = 123.05
                tomato["row"] = i
                tomato["grade"] = 99
                tomato["height_cm"] = 80.5
                tomato["run_id"] = "60942ea7-b3ab-4da0-8a48-a06423e0cab2"
                tomato["image_id"] = "542499b7-1596-4150-bc3e-c3f019020728"

                writer.add_message(
                    channel_id,
                    log_time=time_ns(),
                    data=json.dumps(tomato).encode("utf-8"),
                    publish_time=time_ns(),
                )

            writer.finish()


if __name__ == "__main__":
    main()