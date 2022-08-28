#!/usr/bin/env python3
#pip3 install adafruit-io
# Import library and create instance of REST client.
import json
from time import sleep
from Adafruit_IO import Client
aio = Client(username="leighleighleigh",key='aio_wrSI324kBhMYCBMYioznPAinojqs')

# Get list of feeds.
#feeds = aio.feeds()
# Print out the feed names:
#for f in feeds:
    #print('Feed: {0}'.format(f.name))

data = aio.data("bosl-mqtt")

for d in data:
    try:
        dj = json.loads(d.value)
        if "IMEI" in dj.keys():
            #print(dj)
            print(json.dumps(dj, indent=2, sort_keys=True))
    except:
        pass
