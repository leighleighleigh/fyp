#!/usr/bin/bash
URL=https://cms.leigh.sh/flows/trigger/2f411e8c-ecdf-4b92-9d06-5bd4c2e9f98e
DATA=$(cat data.json)
echo $DATA
curl -X POST -H 'Authorization: Bearer zjl1TenoVD4ld8UHkWCrdzKb6CfnM32Q' -H 'Content-Type: application/json' -d $DATA ${URL}
