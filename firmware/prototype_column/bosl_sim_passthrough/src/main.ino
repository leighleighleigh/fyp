#include <Arduino.h>
#include "fona.h"

void setup()
{
  // Serial.println("BOOT!");
  // simOff();
  simOn();
  moduleSetup();
  fonaSerial.write("ATE1\r\n");
  fonaSerial.flush();
  Serial.begin(9600);
  Serial.flush();
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