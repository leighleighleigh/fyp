#!/usr/bin/bash
#URL=https://***REMOVED***/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f
URL=https://***REMOVED***/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f?access_token=nNgr-OJA-K2cNLkfZWQ0B-Xzlrkb9coN
DATA='{"data":"CMS1,2123123,1234"}'
curl -X POST -H 'Content-Type: application/json' -d $DATA ${URL}
#curl -X POST -H 'Authorization: Bearer zjl1TenoVD4ld8UHkWCrdzKb6CfnM32Q' -H 'Content-Type: application/json' -d $DATA ${URL}
