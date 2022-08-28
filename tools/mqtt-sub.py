#!/usr/bin/env python3
#pip3 install adafruit-io
# Import library and create instance of REST client.
import json
from time import sleep
from Adafruit_IO import Client
import matplotlib.pyplot as plt

# Connect to Adafruit IO
aio = Client(username="leighleighleigh",key='aio_wrSI324kBhMYCBMYioznPAinojqs')

# Build a dictionary of lists, each key is a 'nickname'+IMEI of a board
data = list(aio.data("bosl-mqtt"))
data.reverse()
fleet = {}

for d in data:
    try:
        dj = json.loads(d.value)
        if "IMEI" in dj.keys():
            nn = dj["nickname"]
            if nn not in fleet.keys():
                fleet[nn] = []

            fleet[nn].append(dj["battery_mv"])
            print(json.dumps(dj, indent=2, sort_keys=True))
    except:
        pass

# Write to file
for n in fleet.keys():
    fname = f"{nn}_battery.csv"
    with open(fname,"w") as f:
        #f.write("sample_n, battery_mv\n")
        datan = list(enumerate(fleet[n]))
        for idx, i in datan:
            f.write(f"{idx},{i}\n")

        x,y = [x[0] for x in datan],[x[1] for x in datan]
        plt.plot(x,y)

    plt.show()
