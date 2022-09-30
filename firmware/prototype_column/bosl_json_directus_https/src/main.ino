#include <Arduino.h>
// #include <ArduinoJson.h>
#include "chameleon.h"
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

extern volatile unsigned long timer0_millis;

void reboot() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

void setup()
{
  Serial.begin(115200);
  Serial.println("BOOT!");
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
  pinMode(SMT_TMP,INPUT);
  pinMode(SMT_SOIL,INPUT);
  // Setup FONA
  // setupSIM();
  boolean setupGood = setupFONA();
  if(!setupGood)
  {
    Serial.println("restarting");
    return;
  }

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

  // Sleep for 60 seconds in low-power mode
  // Sleepy(60);
  // reboot();
  delay(10000);
}