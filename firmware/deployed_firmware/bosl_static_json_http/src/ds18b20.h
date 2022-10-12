#include <Arduino.h>
/********************************************************************/
// First we include the libraries
#include <OneWire.h> 
#include <DallasTemperature.h>
/********************************************************************/
// Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 2 
#define ONE_WIRE_BUS_PU 7
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire,ONE_WIRE_BUS_PU);
/********************************************************************/ 

void initTemperatures()
{
  // Start up the library 
  sensors.begin();
  Serial.print("Temperature sensors initialising...");

  uint8_t sensorCount = sensors.getDeviceCount();
  Serial.print("Sensors found: ");
  Serial.println(sensorCount);
}

void readTemperatures(float * tempA, float * tempB)
{
    sensors.requestTemperatures();
    sensors.millisToWaitForConversion(100);
    
    // *tempA = sensors.getTempC(channelA); 
    *tempA = sensors.getTempCByIndex(0); 
    // *tempB = sensors.getTempC(channelB);
    *tempB = sensors.getTempCByIndex(1); 
}