#include <Arduino.h>
#include <ArduinoJson.h>
#include "chameleon.h"
#include "ds18b20.h"
#include "smt.h"
#include <LowPower.h>
#include <avr/wdt.h>
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
    StaticJsonDocument<300> doc;
    // deserializeJson(doc, jsonSchema);
    // Convert the document to an object
    JsonObject obj = doc.to<JsonObject>();

    obj[F("device_name")] = boardName;
    obj[F("imei")] = imei;
    obj[F("batt_mv")] = vBatt;
    obj[F("uptime_s")] = millis()/1000;
    obj[F("chmln_top_ohms")] = upper_sensorResistance;
    obj[F("chmln_bot_ohms")] = lower_sensorResistance;
    obj[F("ds18b20_top_temp_c")] = upper_temp;
    obj[F("ds18b20_bot_temp_c")] = lower_temp;
    obj[F("smt_vwc")] = rawSoil;
    obj[F("smt_temp_c")] = rawTemp;

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
  readChameleon(CHAMELEON_UPPER_A, CHAMELEON_UPPER_B, &upper_rawA, &upper_rawB, &upper_rawAverage, &upper_sensorResistance);
  // Read lower chameleon
  readChameleon(CHAMELEON_LOWER_A, CHAMELEON_LOWER_B, &lower_rawA, &lower_rawB, &lower_rawAverage, &lower_sensorResistance);

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
  Serial.println("Sleeping...");
  Sleepy(60);
  Serial.println("...waking up!!!");
}