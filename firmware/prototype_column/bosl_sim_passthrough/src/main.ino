#include <Arduino.h>
#include "fona.h"

void setup()
{
  Serial.begin(9600);
  // Serial.println("BOOT!");
  simOn();
  moduleSetup();
  fonaSerial.println("ATE0");
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