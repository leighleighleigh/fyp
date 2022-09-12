#include <Arduino.h>
#include "fona.h"

void setup()
{
  Serial.begin(9600);
  // Serial.flush();
  // Serial.println("");
  // Serial.println("");
  // Serial.println("");
  // simOff();
  // simOn();
  moduleSetup();
  // delay(100);
  // fonaSerial.println("ATE1");
  // delay(100);
  // fonaSerial.flush();
}

void loop()
{
  while(Serial.available())
  {
    char c = Serial.read();
    if(c == '!')
    {
      Serial.println("SIM_ON...");
      simOn(); 
      Serial.println("OK");
      break;
    }
    if(c == '@')
    {
      Serial.println("SIM_OFF");
      simOff(); 
      Serial.println("OK");
      break;
    }
    fonaSerial.write(c);
  }

  while(fonaSerial.available())
  {
    Serial.write(fonaSerial.read());
  }
}