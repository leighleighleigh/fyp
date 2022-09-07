#!/usr/bin/bash
URL=https://cms.leigh.sh/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f
DATA="$(jq -nc '.data="1,2,3,4,5,hi"')"
TOKEN='nNgr-OJA-K2cNLkfZWQ0B-Xzlrkb9coN'
MIME="Content-Type: application/json"
#MIME='Content-Type: text/plain'

curl -X POST -H "Authorization: Bearer ${TOKEN}" -H "${MIME}" -d $DATA ${URL}
