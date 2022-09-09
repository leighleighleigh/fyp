#include <Arduino.h>
#include <ArduinoJson.h>

void readSMT(int pinTemp, int pinoSoil, int *rawTemp, int *rawSoil)
{
    // ASSUMES a 10 BIT ADC RESOLUTION
    // Place into forward current mode
    pinMode(pinTemp, INPUT);
    pinMode(pinoSoil, INPUT);

    *rawTemp = analogRead(pinTemp);
    *rawSoil = analogRead(pinoSoil);
}

void readSMTToJSON(int pinTemp, int pinSoil, JsonObject& sensor)
{
    // Read using regular read method
    int rawTemp, rawSoil;
    readSMT(pinTemp,pinSoil,&rawTemp,&rawSoil);

    // Store the results nicAely into a JSON document
    sensor["raw_temp"] = rawTemp;
    sensor["raw_soil"] = rawSoil;
}