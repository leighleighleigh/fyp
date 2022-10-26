#!/usr/bin/bash

# Retrieves a list of sensor files from the website
URL=http://bosl.com.au/IoT/wsudwatch/FYP_SGI/

#curl -X GET -H 'Content-Type: application/json' ${URL}
cd ./downloads/
wget -r -np -nd -A json,csv,old -m -k ${URL} 
ls
cd ../
