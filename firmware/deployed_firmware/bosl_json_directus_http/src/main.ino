#include <Arduino.h>
// #include <ArduinoJson.h>
#include "chameleon.h"
#include "ds18b20.h"
#include "smt.h"
#include <LowPower.h>
#include <avr/wdt.h>

#define DEBUG Serial
#include <utils.h>

#include "fona.h"

// SMT100 analog inputs
#define SMT_TMP A0
#define SMT_SOIL A1

// Chameleon sensor uses two pins
#define CHAMELEON_UPPER_A A2
#define CHAMELEON_UPPER_B A3
#define CHAMELEON_LOWER_A A4
#define CHAMELEON_LOWER_B A5


uint8_t tempChannelA, tempChannelB;

extern volatile unsigned long timer0_millis;

// void reboot()
// {
//   wdt_disable();
//   wdt_enable(WDTO_15MS);
//   while (1)
//   {
//   }
// }

void setup()
{
  Serial.begin(115200);
  Serial.println("BOOT!");
  // Start up the library 
  sensors.begin(); 
  Serial.print("Temperature sensors initialising...");

  uint8_t sensorCount = sensors.getDeviceCount();
  Serial.print("Sensors found: ");
  Serial.println(sensorCount);

  // Get sensor addresses, so that calls to read them are fast!
  getAddresses(&tempChannelA,&tempChannelB);
  Serial.print("Address A: ");
  Serial.println(tempChannelA);
  Serial.print("Address B: ");
  Serial.println(tempChannelB);
}

void Sleepy(uint16_t tsleep)
{ 
  // Sleep Time in seconds
  fonaSerial.flush(); // must run before going to sleep
  Serial.flush(); // ensures that all messages have sent through serial before arduino sleeps

  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); // 8 seconds dosen't work on the 8mhz
  // advance millis timer as it is paused in sleep
  noInterrupts();
  timer0_millis += 4000;
  interrupts();

  while (tsleep >= 8)
  {
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); // 8 seconds dosen't work on the 8mhz
    // advance millis timer as it is paused in sleep
    noInterrupts();
    timer0_millis += 4000;
    interrupts();

    tsleep -= 4;
  }
}

void loop()
{

  pinMode(SMT_TMP, INPUT);
  pinMode(SMT_SOIL, INPUT);
  // Setup FONA
  // setupSIM();
  boolean setupGood = setupFONA();
  if (!setupGood)
  {
    Serial.println("restarting");
    delay(3000);
    return;
  }

  // Read SMT100
  // Read using regular read method
  readSMT(SMT_TMP, SMT_SOIL, &rawTemp, &rawSoil);
  // Read upper chameleon
  readChameleon(CHAMELEON_UPPER_A, CHAMELEON_UPPER_B, &upper_rawA, &upper_rawB, &upper_rawAverage, &upper_sensorResistance);
  // Read lower chameleon
  readChameleon(CHAMELEON_LOWER_A, CHAMELEON_LOWER_B, &lower_rawA, &lower_rawB, &lower_rawAverage, &lower_sensorResistance);

  // Read DS18B20 temperature probe
  readTemperatures(tempChannelA,tempChannelB,&upper_temp,&lower_temp);

  // loopSIM();
  loopFONA();

  // Turn off sim
  shutdownFONA();

  // Sleep for 60 seconds in low-power mode
  delay(3000);
  Sleepy(300);
  // reboot();
  // delay(30000);
}