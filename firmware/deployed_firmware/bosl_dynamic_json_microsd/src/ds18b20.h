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

// uint8_t tempChannelA, tempChannelB;

// void getAddresses(uint8_t * channelA, uint8_t * channelB)
// {
//     uint8_t chA,chB;
//     sensors.getAddress(&chA,0);
//     delay(25);
//     sensors.getAddress(&chB,1);
//     delay(25);
//     *channelA = chA;
//     *channelB = chB;
// }

void initTemperatures()
{
  // Start up the library 
  sensors.begin();
  Serial.print("Temperature sensors initialising...");

  uint8_t sensorCount = sensors.getDeviceCount();
  Serial.print("Sensors found: ");
  Serial.println(sensorCount);

//   // Get sensor addresses, so that calls to read them are fast!
//   getAddresses(&tempChannelA,&tempChannelB);
//   Serial.print("Address A: ");
//   Serial.println(tempChannelA);
//   Serial.print("Address B: ");
//   Serial.println(tempChannelB);
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