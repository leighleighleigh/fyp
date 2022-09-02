#include <Arduino.h>
// #include <ArduinoJson.h>
#include "fona.h"

void setup()
{
  Serial.begin(9600);

  // Setup FONA
  setupFONA();
}

void loop()
{
  loopFONA();
  // Delay so the sensor can settle 
  delay(5000);
}