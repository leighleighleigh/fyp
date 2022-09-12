#include <Arduino.h>
#include "fona.h"

void setup()
{
  Serial.begin(9600);
  Serial.flush();
  Serial.println("");
  Serial.println("");
  Serial.println("");
  // simOff();
  simOn();
  moduleSetup();
  // fonaSerial.write("ATE1\r\n");
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