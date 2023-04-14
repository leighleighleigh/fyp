#include <Arduino.h>

#define CHAMELEON_RESISTOR_KOHM 10 // 10kOhm
#define CHAMELEON_SAMPLE_COUNT 20

// Array of raw readings, used by readChameleonAverage
int rawA_samples[CHAMELEON_SAMPLE_COUNT];
int rawB_samples[CHAMELEON_SAMPLE_COUNT];

void prepareChameleonForSleeping(int pinA, int pinB)
{
    // Sets both pins into a high-impedance state
    // Which prevents charge from flowing through the sensor, such as due to galvanic reactions in the soil
    pinMode(pinA, INPUT);
    pinMode(pinB, INPUT);
    // Delay
    delay(10);
}

void readChameleonRaw(int pinA, int pinB, int *rawA, int *rawB)
{
    // ASSUMES a 10 BIT ADC RESOLUTION

    // Place into forward current mode
    pinMode(pinB, INPUT);
    pinMode(pinA, OUTPUT);
    digitalWrite(pinA, HIGH);
    delayMicroseconds(250); // Delay to let things settle
    // Measure vY, takes 125us
    *rawB = analogRead(pinB);
    // Place into no current mode
    digitalWrite(pinA, LOW);
    delayMicroseconds(250);

    // Place into reverse current mode
    pinMode(pinA, INPUT);
    pinMode(pinB, OUTPUT);
    digitalWrite(pinB, HIGH);
    delayMicroseconds(250); // Delay to let things settle
    // Measure vX, takes 125us
    *rawA = analogRead(pinA);
    // Return to no current mode
    digitalWrite(pinB, LOW);
    delayMicroseconds(250);
}

void readChameleonAverage(int pinA, int pinB, int *rawA, int *rawB, float *rawAvg, float *sensorResistanceRaw)
{
    // Make multiple readings of the chameleon sensor, storing these raw values into the arrays
    for (int i = 0; i < CHAMELEON_SAMPLE_COUNT; i++)
    {
        readChameleonRaw(pinA, pinB, &rawA_samples[i], &rawB_samples[i]);
    }

    // Put sensor to slee
    prepareChameleonForSleeping(pinA, pinB);

    // Now average the individual rawA and rawB values, which will be returned via rawA/rawB
    *rawA = 0;
    *rawB = 0;
    for (int i = 0; i < CHAMELEON_SAMPLE_COUNT; i++)
    {
        *rawA += rawA_samples[i];
        *rawB += rawB_samples[i];
    }
    *rawA /= CHAMELEON_SAMPLE_COUNT;
    *rawB /= CHAMELEON_SAMPLE_COUNT;

    // Take the mean of rawA and rawB
    *rawAvg = float((*rawA + *rawB) / 2);

    // Perform a basic value-check on the rawAvg value
    if (*rawAvg < 2) // Very low value indicates open-circuit, or very high resistance
    {
        *sensorResistanceRaw = float(9999);
    }else{
        // Make code a little cleaner
        float avg = *rawAvg;
        // Now we can calculate the resistance of the sensor
        // *sensorResistanceRaw = float(10) * (1023 - *rawAvg) / *rawAvg;
        *sensorResistanceRaw = float(float(CHAMELEON_RESISTOR_KOHM) * (1023.0f - avg) / avg);
    }

    // Final step is to multiply by 1000, to convert from kOhm to Ohm.
    *sensorResistanceRaw *= 1000;
}
