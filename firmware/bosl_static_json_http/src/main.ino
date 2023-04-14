#include <Arduino.h>
#include <ArduinoJson.h>
#include "chameleon.h"
#include "ds18b20.h"
#include "smt.h"
#include <LowPower.h>
#include <avr/wdt.h>
#include <utils.h>
#include "fona.h"

// Sleep time
#ifndef SLEEP_TIME
  #define SLEEP_TIME 600
#endif

// Give sleep time an actual variable, so it can be modified.
uint32_t boslSleepDuration = SLEEP_TIME;

// Undefine to use normal delay() in sleep loop.
// It seems that the Sleepy() function sometimes causes boards to go offline,
// possibly due to a power-brownout during low power mode.
// #define BOSL_DEEPSLEEP 1


// SMT100 analog inputs
#define SMT_TMP A0
#define SMT_SOIL A1

// Chameleon sensor uses two pins
#define CHAMELEON_UPPER_A A2
#define CHAMELEON_UPPER_B A3
#define CHAMELEON_LOWER_A A4
#define CHAMELEON_LOWER_B A5

extern volatile unsigned long timer0_millis;

// GLOBAL SENSOR READINGS
int rawTemp, rawSoil;
int lower_rawA, lower_rawB;
int upper_rawA, upper_rawB;
float upper_temp, lower_temp;
float lower_rawAverage, lower_sensorResistance;
float upper_rawAverage, upper_sensorResistance;

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
}

void Sleepy(uint16_t tsleep)
{
  // Sleep Time in seconds
  fonaSerial.flush(); // must run before going to sleep
  Serial.flush();     // ensures that all messages have sent through serial before arduino sleeps

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

void sendPOST()
{
    StaticJsonDocument<200> doc;
    // deserializeJson(doc, jsonSchema);
    // Convert the document to an object
    JsonObject obj = doc.to<JsonObject>();

    obj[F("id")] = boardName;
    // obj[F("imei")] = imei;
    obj[F("imei")] = imei;
    obj[F("bat")] = vBatt;
    // obj[F("uptime_s")] = millis()/1000;
    // obj[F("uR")] = upper_sensorResistance;
    // obj[F("lR")] = lower_sensorResistance;
    // Changed to report the RAW values of everything.
    // This helps identify issues with wiring.
    obj[F("uCHA")] = upper_rawA;
    obj[F("uCHB")] = upper_rawB;
    obj[F("lCHA")] = lower_rawA;
    obj[F("lCHB")] = lower_rawB;
    // Digital temp sensors
    obj[F("uT")] = upper_temp;
    obj[F("lT")] = lower_temp;
    // Raw ADC readings
    obj[F("smtVWC")] = rawSoil;
    obj[F("smtT")] = rawTemp;

    int counter = 0;
    while (counter < 3 && !fona.postData("POST", URL, obj, token))
    {
        Serial.println(F("Failed to complete HTTP POST..."));
        counter++;
        delay(1000);
    }
}

void loop()
{
  initTemperatures();
  // Read DS18B20 temperature probe
  // lower is index 0, upper is index 1.
  // This is ensured by cable length of the sensors - with the lower sensor having a shorter cable.
  // Read digital temperature probes
  readTemperatures(&lower_temp, &upper_temp);
  // Read SMT100 analogue values
  readSMT(SMT_TMP, SMT_SOIL, &rawTemp, &rawSoil);
  // Read upper chameleon
  readChameleonAverage(CHAMELEON_UPPER_A, CHAMELEON_UPPER_B, &upper_rawA, &upper_rawB, &upper_rawAverage, &upper_sensorResistance);
  // Read lower chameleon
  readChameleonAverage(CHAMELEON_LOWER_A, CHAMELEON_LOWER_B, &lower_rawA, &lower_rawB, &lower_rawAverage, &lower_sensorResistance);

  // Setup FONA
  boolean setupGood = setupFONA();
  // Returns false if we couldn't find the fona
  if (!setupGood)
  {
    Serial.println("restarting");
    delay(3000);
    return;
  }

  // Send the POST
  sendPOST();

  // Turn off sim
  shutdownFONA();

  // Sleep for 60 seconds in low-power mode
  delay(1000);

  #ifdef BOSL_DEEPSLEEP
    Serial.println("Sleeping using LowPower sleep");
    Sleepy(boslSleepDuration);
  #else
    Serial.println("Sleeping using delay()");
    uint32_t sleept = boslSleepDuration * 1000;
    delay(sleept);
  #endif
}