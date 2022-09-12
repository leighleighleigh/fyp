#include <Arduino.h>
#include "fona.h"

unsigned long lastcmdtime = millis();

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
    // IF we havnt received a command in a while
    if(millis() - lastcmdtime > 5000)
    {
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
    }
    fonaSerial.write(c);
    lastcmdtime = millis();
  }

  while(fonaSerial.available())
  {
    lastcmdtime = millis();
    Serial.write(fonaSerial.read());
  }
}