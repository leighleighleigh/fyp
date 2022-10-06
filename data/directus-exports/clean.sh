#!/usr/bin/bash

./clean_sensor_readings.py --input-file "column_sensors 20221007-84218.json" --output-file "column_sensors_cleaned.json"

tac column_sensors_cleaned.json > cleaned.csv

