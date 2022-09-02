#include <Arduino.h>
#include "Adafruit_FONA.h" // https://github.com/botletics/SIM7000-LTE-Shield/tree/master/Code
// #include "Adafruit_MQTT.h"
// #include "Adafruit_MQTT_FONA.h"
// #include "ArduinoJson.h"
#include <schema.h>
#include <SoftwareSerial.h>

#define SIMCOM_7000
#define FONA_PWRKEY 4
#define FONA_DTR 5 // Connect with solder jumper
#define FONA_TX 9  // Microcontroller RX
#define FONA_RX 8  // Microcontroller TX

SoftwareSerial fonaSerial = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

#define DIRECTUS_URL "http://***REMOVED***/items/column_sensors"
#define DIRECTUS_TOKEN "PXzLIINzWiEsd13j0eMloz2QbtB7LzAs"

/****************************** OTHER STUFF ***************************************/
char imei[16] = {0};
uint8_t fonaType;
String body;

bool netStatus();
void moduleSetup();

void setupFONA()
{

    fona.powerOn(FONA_PWRKEY);
    moduleSetup();

    fona.setFunctionality(1);

    // fona.setNetworkSettings(F("mdata.net.au"));
    fona.setNetworkSettings(F("mdata.net.au"));

    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made
    while (!netStatus())
    {
        Serial.println(F("Failed to connect to cell network, retrying..."));
        delay(5000); // Retry every 2s
    }
    Serial.println(F("Connected to cell network!"));
}

void loopFONA()
{
    uint16_t vbatt;
    fona.getBattVoltage(&vbatt);
    char body[512];

    sprintf(body, column_data_schema, "BW3", String(imei), vbatt, String(""), 1,2,3,4,5,6,7,8,9,10,11,12,13,14);
    // doc["bosl_name"] = "BW3";
    // doc["bosl_imei"] = String(imei);
    // doc["bosl_battery_mv"] = vbatt;

    // // Top sensors
    // doc["chmln_top_raw_a"] = 300;
    // doc["chmln_top_raw_b"] = 250;
    // doc["chmln_top_raw_avg"] = 6000;
    // doc["chmln_top_resistance"] = 200;
    // doc["smt100_top_vwc"] = 10.5;
    // doc["smt100_top_temperature"] = 24;

    // // Bottom sensors
    // doc["chmln_bot_raw_a"] = 300;
    // doc["chmln_bot_raw_b"] = 250;
    // doc["chmln_bot_raw_avg"] = 6000;
    // doc["chmln_bot_resistance"] = 22;

    // doc["ds18b20_bot_temperature"] = 22;

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


    char URL[64];
    char TOKENSTR[64];

    sprintf(TOKENSTR, "%s", DIRECTUS_TOKEN); 
    sprintf(URL, "%s", DIRECTUS_URL); 

    // serializeJson(doc, Serial);
    Serial.print(strlen(body));
    Serial.println();
    Serial.print(body);
    Serial.println();
    
    fona.postData("POST", URL, body, TOKENSTR);
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
