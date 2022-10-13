#!/usr/bin/bash

# Retrieves a list of sensor files from the website
URL=http://bosl.com.au/IoT/wsudwatch/FYP_SGI/

# Store current directory
WKD=$(pwd)

# Enters a temporary folder
TMPD=$(mktemp -d)

# Enter directory and download logs there
cd $TMPD
wget -r -np -nd -A json -m -k ${URL} 

# Merge all logs together, which is done easily as they are self-contained in JSON lines
echo *.json| xargs cat > ${WKD}/merged.json

# Exit tempdir
cd $WKD

# Delete tempdir
rm -r $TMPD
