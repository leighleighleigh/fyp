#!/usr/bin/bash

# After running ./get_web_data.sh,
# all the data will be in the ./data/ folder.
# This script takes all of the '.json' or '.json.old' files, and merges them into a single file (merged.json).

# OUTFILE
OUTFILE="merged.json"

# If a second argument was provided, use it as output filename
if [ $# -eq 2 ]; then
    OUTFILE=$2
fi

# Store current directory
WKD=$(pwd)

# Merge all logs together, which is done easily as they are self-contained in JSON lines
echo ./data/*.json*| xargs cat > ${WKD}/${OUTFILE}

# Exit tempdir
cd $WKD
