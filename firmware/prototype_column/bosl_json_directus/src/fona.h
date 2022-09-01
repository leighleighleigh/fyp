#include <Arduino.h>
#include "Adafruit_FONA.h" // https://github.com/botletics/SIM7000-LTE-Shield/tree/master/Code
// #include "Adafruit_MQTT.h"
// #include "Adafruit_MQTT_FONA.h"
#include "ArduinoJson.h"
#include <SoftwareSerial.h>

#define SIMCOM_7000
#define FONA_PWRKEY 4
#define FONA_DTR 5 // Connect with solder jumper
#define FONA_TX 9  // Microcontroller RX
#define FONA_RX 8  // Microcontroller TX

SoftwareSerial fonaSerial = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

#define DIRECTUS_FLOW_URL "https://***REMOVED***/flows/trigger/2f411e8c-ecdf-4b92-9d06-5bd4c2e9f98e"
#define DIRECTUS_TOKEN "PXzLIINzWiEsd13j0eMloz2QbtB7LzAs"

/****************************** OTHER STUFF ***************************************/
char imei[16] = {0};
uint8_t fonaType;

DynamicJsonDocument doc(1024);

// Method 'headers'
bool netStatus();
void moduleSetup();

void setupFONA()
{
    fona.powerOn(FONA_PWRKEY);
    moduleSetup();

    fona.setFunctionality(1);

    // fona.setNetworkSettings(F("mdata.net.au"));
    fona.setNetworkSettings(F("mdata.net.au"));

    // turn GPRS off
    // if (!fona.enableGPRS(false))
    // {
    //     Serial.println(F("Failed to turn off GPRS..."));
    //     delay(2000);
    // }

    // turn GPRS on
    // if (!fona.enableGPRS(true))
    // {
    //     Serial.println(F("Failed to turn on GPRS..."));
    //     delay(2000);
    // }

    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made
    while (!netStatus())
    {
        Serial.println(F("Failed to connect to cell network, retrying..."));
        delay(2000); // Retry every 2s
    }
    Serial.println(F("Connected to cell network!"));
}

// void readGPStoJSON(JsonObject &gps_obj)
// {
//     // Get a fix on location, try every 2s
//     // Use the top line if you want to parse UTC time data as well, the line below it if you don't care
//     //  while (!fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude, &year, &month, &day, &hour, &minute, &second)) {
//     if (!fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude))
//     {
//         Serial.println(F("Failed to get GPS location..."));
//     }

//     // Store the results nicely into a JSON document
//     gps_obj["lat"] = latitude;
//     gps_obj["lon"] = longitude;
//     gps_obj["ele"] = altitude;
//     // gps_obj["lat"] = -37.78;
//     // gps_obj["lon"] = 144.944;
//     // gps_obj["ele"] = 10.0;
// }

void loopFONA()
{
    // The json document variable
    JsonObject root = doc.to<JsonObject>();

    JsonObject deviceObject = root.createNestedObject("device");
    uint16_t vBatt;
    deviceObject["nickname"] = "BW3";
    deviceObject["battery_mv"] = vBatt;
    deviceObject["IMEI"] = String(imei);
    deviceObject["uptime"] = float(millis() / 1000);

    JsonObject upperChameleon = root.createNestedObject("upper_chameleon");
    upperChameleon["pin_a_raw"] = 200;
    upperChameleon["pin_b_raw"] = 300;
    upperChameleon["raw_average"] = 250;
    upperChameleon["resistance"] = 6000;

    JsonObject lowerChameleon = root.createNestedObject("lower_chameleon");
    lowerChameleon["pin_a_raw"] = 200;
    lowerChameleon["pin_b_raw"] = 300;
    lowerChameleon["raw_average"] = 250;
    lowerChameleon["resistance"] = 6000;

    JsonObject upperTemp = root.createNestedObject("upper_temperature");
    upperTemp["temperature"] = 22;
    JsonObject lowerTemp = root.createNestedObject("lower_temperature");
    lowerTemp["temperature"] = 22;

    // Open wireless connection if not already activated
    if (!fona.wirelessConnStatus())
    {
        while (!fona.openWirelessConnection(true))
        {
            Serial.println(F("Failed to enable connection, retrying..."));
            delay(2000); // Retry every 2s
        }
        Serial.println(F("Enabled data!"));
    }
    else
    {
        Serial.println(F("Data already enabled!"));
    }

    // Post data to website
    uint16_t statuscode;
    int16_t length;
    char data[250];

    serializeJson(doc, data);

    if (!fona.HTTP_POST_start(DIRECTUS_FLOW_URL, F("application/json"), (uint8_t *)data, strlen(data), &statuscode, (uint16_t *)&length))
    {
        Serial.println("Failed!");
    }
    Serial.println(F("\n****"));
    fona.HTTP_POST_end();
}

void moduleSetup()
{
    // SIM7000 takes about 3s to turn on and SIM7500 takes about 15s
    // Press Arduino reset button if the module is still turning on and the board doesn't find it.
    // When the module is on it should communicate right after pressing reset

    // Software serial:
    fonaSerial.begin(115200); // Default SIM7000 shield baud rate

    Serial.println(F("Configuring to 9600 baud"));
    fonaSerial.println("AT+IPR=9600"); // Set baud rate
    delay(100);                        // Short pause to let the command run
    fonaSerial.begin(9600);
    if (!fona.begin(fonaSerial))
    {
        Serial.println(F("Couldn't find FONA"));
        while (1)
            ; // Don't proceed if it couldn't find the device
    }

    fonaType = fona.type();
    Serial.println(F("FONA is OK"));
    Serial.print(F("Found "));
    switch (fonaType)
    {
    case SIM800L:
        Serial.println(F("SIM800L"));
        break;
    case SIM800H:
        Serial.println(F("SIM800H"));
        break;
    case SIM808_V1:
        Serial.println(F("SIM808 (v1)"));
        break;
    case SIM808_V2:
        Serial.println(F("SIM808 (v2)"));
        break;
    case SIM5320A:
        Serial.println(F("SIM5320A (American)"));
        break;
    case SIM5320E:
        Serial.println(F("SIM5320E (European)"));
        break;
    case SIM7000:
        Serial.println(F("SIM7000"));
        break;
    case SIM7070:
        Serial.println(F("SIM7070"));
        break;
    case SIM7500:
        Serial.println(F("SIM7500"));
        break;
    case SIM7600:
        Serial.println(F("SIM7600"));
        break;
    default:
        Serial.println(F("???"));
        break;
    }

    // Print module IMEI number.
    uint8_t imeiLen = fona.getIMEI(imei);
    if (imeiLen > 0)
    {
        Serial.print("Module IMEI: ");
        Serial.println(imei);
    }
}

bool netStatus()
{
    int n = fona.getNetworkStatus();

    Serial.print(F("Network status "));
    Serial.print(n);
    Serial.print(F(": "));
    if (n == 0)
        Serial.println(F("Not registered"));
    if (n == 1)
        Serial.println(F("Registered (home)"));
    if (n == 2)
        Serial.println(F("Not registered (searching)"));
    if (n == 3)
        Serial.println(F("Denied"));
    if (n == 4)
        Serial.println(F("Unknown"));
    if (n == 5)
        Serial.println(F("Registered roaming"));

    if (!(n == 1 || n == 5))
        return false;
    else
        return true;
}
