#!/usr/bin/bash

# POSTs data.json to a URL

#URL=https://***REMOVED***/items/column_sensors
URL=http://bosl.com.au/IoT/wsudwatch/FYP_SGI/log.php

DATA=$(cat data.json)
#curl -X POST -H 'Authorization: Bearer zjl1TenoVD4ld8UHkWCrdzKb6CfnM32Q' -H 'Content-Type: application/json' -d $DATA ${URL}
curl -X POST -H 'Content-Type: application/json' -d $DATA ${URL}
