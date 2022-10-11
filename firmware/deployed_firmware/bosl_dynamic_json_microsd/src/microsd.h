#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>

// #define PIN_SPI_CS 13

File myFile;
boolean sdFound = false;

void setupSD()
{
    sdFound = false;
    if (!SD.begin(PIN_SPI_SS, PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_SCK))
    {
        Serial.println(F("SD CARD FAILED, OR NOT PRESENT!"));
        while (1)
            ; // don't do anything more:
    }

    Serial.println(F("SD CARD INITIALIZED."));
    sdFound = true;

    if (!SD.exists("arduino.txt"))
    {
        Serial.println(F("arduino.txt doesn't exist. Creating arduino.txt file..."));
        // create a new file by opening a new file and immediately close it
        myFile = SD.open("arduino.txt", FILE_WRITE);
        myFile.close();
    }

    // recheck if file is created or not
    if (SD.exists("arduino.txt"))
        Serial.println(F("arduino.txt exists on SD Card."));
    else
        Serial.println(F("arduino.txt doesn't exist on SD Card."));
}

void logSD(char * data, size_t len)
{
    if(sdFound){
        Serial.println("Opening arduino.txt");
        myFile = SD.open("arduino.txt", FILE_WRITE);

        Serial.println("Writing...");
        // Serial.write(data, len);
        // Serial.println();

        myFile.write(data, len);
        myFile.write("\n");

        Serial.println("Closing arduino.txt");
        myFile.close();
    }
}