#include <Arduino.h>
#include <ArduinoJson.h>
#include "chameleon.h"
#include "smt.h"
#include "fona.h"

// SMT100 analog inputs
#define SMT_TMP A0
#define SMT_SOIL A1

// Chameleon sensor uses two pins
#define CHMLN_0_A A2
#define CHMLN_0_B A3
#define CHMLN_1_A A4
#define CHMLN_1_B A5

long sampleCount = 0;

void setup()
{
  Serial.begin(9600);

  pinMode(SMT_TMP,INPUT);
  pinMode(SMT_SOIL,INPUT);
  // Wait for connection
  while(!Serial) continue;

  // Setup FONA
  setupFONA();
}

void readSensorsToJSON()
{
  // The json document variable 
  StaticJsonDocument<256> doc;
  JsonObject root = doc.to<JsonObject>();

  // Add timestamp information to the json document
  doc["sample_n"] = sampleCount;
  doc["sample_ms"] = millis();

  // Read chameleon 0
  JsonObject chmn0 = root.createNestedObject("chameleon_lower");
  readChameleonToJSON(CHMLN_0_A,CHMLN_0_B,chmn0);

  // Read chameleon 1
  JsonObject chmn1 = root.createNestedObject("chameleon_upper");
  readChameleonToJSON(CHMLN_1_A,CHMLN_1_B,chmn1);

  // Read SMT100 analog lines
  JsonObject smt0 = root.createNestedObject("smt_upper");
  readSMTToJSON(SMT_TMP,SMT_SOIL,smt0);

  // Write the JSON output to serial
  serializeJson(doc, Serial);
  Serial.println();

  // Increment sample counter
  sampleCount++;
}

void loop()
{
  loopFONA();
  // Delay so the sensor can settle 
  delay(1000);
}