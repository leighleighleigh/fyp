#!/usr/bin/bash

# Retrieves all sensor data from the website, including .old data
URL=http://bosl.com.au/IoT/wsudwatch/FYP_SGI/

cd ./data/
wget -r -np -nd -A json,csv,old -m -k ${URL} 
ls
cd ../
