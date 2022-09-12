#!/usr/bin/env python3

import serial
import time

ser = serial.Serial('/dev/ttyBOSL', 115200, timeout=1)

def send(data):
    try:
        ser.write(data)
    except Exception as e:
        print("Couldn't send data to serial port: %s" % str(e))
    else:
        try:
            data = ser.read(1)
        except Exception as e:
            print("Couldn't read data from serial port: %s" % str(e))
        else:
            if data:  # If data = None, timeout occurr
                n = ser.inWaiting()
                if n > 0: data += ser.read(n)
                return data

while(1):
    print(send(b'AT\r\n'))
    time.sleep(1.0)
