#!/usr/bin/bash

# Retrieves a list of sensor files from the website. By default, uses the root directory:
URL=http://bosl.com.au/IoT/wsudwatch/FYP_SGI/

# If an input argument was provided, we use that as the URL instead!
if [ $# -eq 1 ]; then
    # Check that 'http' is in the argument provided:
    if [[ $1 == *"http"* ]]; then
        URL=$1
    else
        echo "Argument must be a URL of a file directory, containing JSON files."
        exit 1
    fi
fi

# OUTFILE
OUTFILE="merged.json"

# If a second argument was provided, use it as output filename
if [ $# -eq 2 ]; then
    OUTFILE=$2
fi

# Store current directory
WKD=$(pwd)

# Enters a temporary folder
TMPD=$(mktemp -d)

# Enter directory and download logs there
cd $TMPD
wget -r -np -nd -A json -m -k ${URL} 

# Merge all logs together, which is done easily as they are self-contained in JSON lines
echo *.json| xargs cat > ${WKD}/${OUTFILE}

# Exit tempdir
cd $WKD

# Delete tempdir
rm -r $TMPD
