#include <Arduino.h>
/********************************************************************/
// First we include the libraries
#include <OneWire.h> 
#include <DallasTemperature.h>
/********************************************************************/
// Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 2 
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 

void getAddresses(uint8_t * channelA, uint8_t * channelB)
{
    sensors.getAddress(channelA,0);
    sensors.getAddress(channelB,1);
}

void readTemperatures(uint8_t channelA, uint8_t channelB, float * tempA, float * tempB)
{
    sensors.requestTemperatures();
    *tempA = sensors.getTempC(&channelA); 
    *tempB = sensors.getTempC(&channelB);
}