#include <Arduino.h>
// #include <ArduinoJson.h>

void readChameleon(int pinA, int pinB, int *rawA, int *rawB, float *rawAvg, float *sensorResistance)
{
    // ASSUMES a 10 BIT ADC RESOLUTION

    // Place into forward current mode
    pinMode(pinB, INPUT);
    pinMode(pinA, OUTPUT);
    digitalWrite(pinA, HIGH);
    // Measure vY, takes 125us
    *rawB = analogRead(pinB);
    // Place into no current mode
    digitalWrite(pinA, LOW);
    delayMicroseconds(250);

    // Place into reverse current mode
    pinMode(pinA, INPUT);
    pinMode(pinB, OUTPUT);
    digitalWrite(pinB, HIGH);
    // Measure vX, takes 125us
    *rawA = analogRead(pinA);
    // Return to no current mode
    digitalWrite(pinB, LOW);

    // Average the two readings
    *rawAvg = (*rawB + *rawA) / 2;
    // Calculate the result in KOhms
    *sensorResistance = float(10) * (1023 - *rawAvg) / *rawAvg;
    // Multiply by 1000 for Ohms
    *sensorResistance *= 1000;
}

// float applyTemperatureCorrection(float rS, float tempC)
// {
//   // Positive temperature coefficient, neutral at 22C.
//   return rS * (1 + (tempC - 22) * 0.018);
// }

// void readChameleonToJSON(int pinA, int pinB, JsonObject& sensor)
// {
//     // Read using regular read method
//     int rawA, rawB;
//     float rawAverage, sensorResistance;
//     readChameleon(pinA,pinB,&rawA,&rawB,&rawAverage,&sensorResistance);

//     // Store the results nicAely into a JSON document
//     // sensor[String("name")] = String("chameleon_") + String(pinA) + "_" + String(pinB);
//     sensor["raw_a"] = rawA;
//     sensor["raw_b"] = rawB;
//     sensor["raw_average"] = rawAverage;
//     sensor["resistance"] = sensorResistance;
// }