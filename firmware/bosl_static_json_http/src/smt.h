#include <Arduino.h>

void readSMT(int pinTemp, int pinSoil, int *rawTemp, int *rawSoil)
{
    // ASSUMES a 10 BIT ADC RESOLUTION
    // Place into forward current mode
    pinMode(pinTemp,  INPUT);
    pinMode(pinSoil,  INPUT);

    // Delay to settle pins
    delay(10);
    
    // Read twice incase first reading is bugged
    *rawTemp = analogRead(pinTemp);
    *rawTemp = analogRead(pinTemp);

    *rawSoil = analogRead(pinSoil);
    *rawSoil = analogRead(pinSoil);
}
