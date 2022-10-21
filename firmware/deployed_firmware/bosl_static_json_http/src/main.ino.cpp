# 1 "/tmp/tmpu9scqwd5"
#include <Arduino.h>
# 1 "/home/leigh/Documents/GitHub/fyp/firmware/deployed_firmware/bosl_static_json_http/src/main.ino"
#include <Arduino.h>
#include <ArduinoJson.h>
#include "chameleon.h"
#include "ds18b20.h"
#include "smt.h"
#include <LowPower.h>
#include <avr/wdt.h>
#include <utils.h>
#include "fona.h"


#ifndef SLEEP_TIME
  #define SLEEP_TIME 600
#endif


uint32_t boslSleepDuration = SLEEP_TIME;
# 26 "/home/leigh/Documents/GitHub/fyp/firmware/deployed_firmware/bosl_static_json_http/src/main.ino"
#define SMT_TMP A0
#define SMT_SOIL A1


#define CHAMELEON_UPPER_A A2
#define CHAMELEON_UPPER_B A3
#define CHAMELEON_LOWER_A A4
#define CHAMELEON_LOWER_B A5

extern volatile unsigned long timer0_millis;


int rawTemp, rawSoil;
int lower_rawA, lower_rawB;
int upper_rawA, upper_rawB;
float upper_temp, lower_temp;
float lower_rawAverage, lower_sensorResistance;
float upper_rawAverage, upper_sensorResistance;
# 54 "/home/leigh/Documents/GitHub/fyp/firmware/deployed_firmware/bosl_static_json_http/src/main.ino"
void setup();
void Sleepy(uint16_t tsleep);
void sendPOST();
void loop();
#line 54 "/home/leigh/Documents/GitHub/fyp/firmware/deployed_firmware/bosl_static_json_http/src/main.ino"
void setup()
{
  Serial.begin(115200);
  Serial.println("BOOT!");
}

void Sleepy(uint16_t tsleep)
{

  fonaSerial.flush();
  Serial.flush();

  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);

  noInterrupts();
  timer0_millis += 4000;
  interrupts();

  while (tsleep >= 8)
  {
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);

    noInterrupts();
    timer0_millis += 4000;
    interrupts();

    tsleep -= 4;
  }
}

void sendPOST()
{
    StaticJsonDocument<200> doc;


    JsonObject obj = doc.to<JsonObject>();

    obj[F("id")] = boardName;

    obj[F("imei")] = imei;
    obj[F("bat")] = vBatt;





    obj[F("uCHA")] = upper_rawA;
    obj[F("uCHB")] = upper_rawB;
    obj[F("lCHA")] = lower_rawA;
    obj[F("lCHB")] = lower_rawB;

    obj[F("uT")] = upper_temp;
    obj[F("lT")] = lower_temp;

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




  readTemperatures(&lower_temp, &upper_temp);

  readSMT(SMT_TMP, SMT_SOIL, &rawTemp, &rawSoil);

  readChameleonRaw(CHAMELEON_UPPER_A, CHAMELEON_UPPER_B, &upper_rawA, &upper_rawB);

  readChameleonRaw(CHAMELEON_LOWER_A, CHAMELEON_LOWER_B, &lower_rawA, &lower_rawB);


  boolean setupGood = setupFONA();

  if (!setupGood)
  {
    Serial.println("restarting");
    delay(3000);
    return;
  }


  sendPOST();


  shutdownFONA();


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