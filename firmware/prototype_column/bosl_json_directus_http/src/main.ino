#include <Arduino.h>
#include <ArduinoJson.h>
#include "chameleon.h"
#include "smt.h"
#include <LowPower.h>

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

extern volatile unsigned long timer0_millis;

void setup()
{
  simOff();
  simOn();

  Serial.begin(115200);
  pinMode(SMT_TMP,INPUT);
  pinMode(SMT_SOIL,INPUT);
  // Setup FONA
  // setupSIM();
  setupFONA(true);
}

void Sleepy(uint16_t tsleep){ //Sleep Time in seconds
    
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); //8 seconds dosen't work on the 8mhz
    //advance millis timer as it is paused in sleep
    noInterrupts();
    timer0_millis += 4000;
    interrupts(); 
    
    
    while(tsleep >= 8){
        LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); //8 seconds dosen't work on the 8mhz
        //advance millis timer as it is paused in sleep
        noInterrupts();
        timer0_millis += 4000;
        interrupts();
        
        tsleep -= 4;
    }
}

void loop()
{
  // Serial.begin(115200);
  pinMode(SMT_TMP,INPUT);
  pinMode(SMT_SOIL,INPUT);
  // Setup FONA
  // setupSIM();
  setupFONA(false);

  // Read SMT100
  // Read using regular read method
  readSMT(SMT_TMP,SMT_SOIL,&rawTemp,&rawSoil);
  // Read upper chameleon
  readChameleon(CHAMELEON_UPPER_A,CHAMELEON_UPPER_B,&upper_rawA,&upper_rawB,&upper_rawAverage,&upper_sensorResistance);
  // Read lower chameleon
  readChameleon(CHAMELEON_LOWER_A,CHAMELEON_LOWER_B,&lower_rawA,&lower_rawB,&lower_rawAverage,&lower_sensorResistance);

  // loopSIM();
  loopFONA();

  // Turn off sim
  shutdownFONA();

  // Sleep for 60 seconds
  Sleepy(60);
}