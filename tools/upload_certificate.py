#!/usr/bin/env python3

import serial
import time
import sys

ser = serial.Serial('/dev/ttyBOSL', 9600, timeout=5)

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

def AT(cmd : str = "",reply : str = "OK", retry=False, waittime=1.0):
    a = f"AT{cmd}\r\n"
    print(f"> AT{cmd}")
    ret = send(bytes(a,'ascii'))
    time.sleep(waittime)

    if ret:
        retdec = ret.decode("ascii").strip()
        print(retdec)
        if reply in retdec:
            return True
        else:
            return False
    else:
        if retry:
            print("Retrying...")
            AT(cmd,reply,retry)
        else:
            return False


AT(retry=True)
AT("E0",retry=True)
AT("+CFSTERM")
sys.exit()

AT("+CFSINIT",waittime=5.0)

fname = "fullchain.pem"
flen = 0
fdata = bytes()
with open(fname,"rb") as f:
    fdata = f.read()
    flen = len(fdata)

AT(f"+CFSWFILE,0,\"{fname}\",0,{flen},10000",waittime=5.0)
send(fdata)
time.sleep(1.0)
AT("+CFSTERM")
AT("+CFSINIT",waittime=5.0)
AT(f"+CFSRFILE,0,\"{fname}\",0,{flen},0")
