#!/usr/bin/bash
URL=https://cms.leigh.sh/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f
DATA='{"data":"1,2,3,4,5,hi"}'

curl -X POST -H 'Authorization: Bearer zjl1TenoVD4ld8UHkWCrdzKb6CfnM32Q' -H 'Content-Type: application/json' -d $DATA ${URL}
