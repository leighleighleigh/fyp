#include <Arduino.h>
#include <ArduinoJson.h>
#include "chameleon.h"
#include "smt.h"

#define DEBUG Serial
#include <utils.h>

#include "fona.h"
// #include "sim.h"

// SMT100 analog inputs
#define SMT_TMP A0
#define SMT_SOIL A1

// Chameleon sensor uses two pins
#define CHMLN_0_A A2
#define CHMLN_0_B A3
#define CHMLN_1_A A4
#define CHMLN_1_B A5

void setup()
{
  Serial.begin(115200);

  pinMode(SMT_TMP,INPUT);
  pinMode(SMT_SOIL,INPUT);
  // Wait for connection
  // while(!Serial) continue;

  // Setup FONA
  // setupSIM();
  setupFONA();
}

void loop()
{
  // loopSIM();
  loopFONA();
  // Sleep for 60 seconds
  delay(60000);
}
