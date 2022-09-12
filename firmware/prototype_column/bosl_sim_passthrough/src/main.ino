#include <Arduino.h>
#include <avr/wdt.h>
#define DEBUG Serial
#include "fona.h"

void reboot() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void setup()
{
  // Serial.println("BOOT!");
  simOn();
  moduleSetup();

  Serial.begin(115200);
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