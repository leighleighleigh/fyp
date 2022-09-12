#include <Arduino.h>
#include "fona.h"

void setup()
{
  Serial.begin(9600);
  // Serial.flush();
  // Serial.println("");
  // Serial.println("");
  Serial.println("");
  // simOff();
  simOn();
  moduleSetup();
  delay(100);
  fonaSerial.println("ATE1");
  delay(100);
  // fonaSerial.flush();
}

void loop()
{
  if(fonaSerial.available())
  {
    Serial.write(fonaSerial.read());
  }
  if(Serial.available())
  {
    fonaSerial.write(Serial.read());
  }
}