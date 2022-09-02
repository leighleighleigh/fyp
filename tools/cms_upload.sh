#!/usr/bin/bash
URL=https://cms.leigh.sh/items/column_sensors
DATA=$(cat data.json)
#echo $DATA
curl -X POST -H 'Authorization: Bearer zjl1TenoVD4ld8UHkWCrdzKb6CfnM32Q' -H 'Content-Type: application/json' -d $DATA ${URL}
