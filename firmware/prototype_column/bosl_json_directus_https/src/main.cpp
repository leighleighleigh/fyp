#include <Arduino.h>
#include "fona.h"

void setup()
{
  // Setup FONA
  setupFONA();
}

void loop()
{
  loopFONA();
  // Delay so the sensor can settle 
  delay(5000);
}