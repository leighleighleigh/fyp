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

/************************* MQTT SETUP *********************************/
// MQTT setup (if you're using it, that is)
// For Adafruit IO:
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "leighleighleigh"
#define AIO_BOARDNAME "BW3"
#define AIO_KEY "aio_wrSI324kBhMYCBMYioznPAinojqs"

/****************************** OTHER STUFF ***************************************/
char imei[16] = {0}; // Use this for device ID
uint8_t type;
float latitude, longitude, speed_kph, heading, altitude, second;
uint16_t year;
uint16_t vBatt;
uint8_t month, day, hour, minute;

// char PIN[5] = "1234"; // SIM card PIN

// NOTE: Keep the buffer sizes as small as possible, espeially on
// Arduino Uno which doesn't have much computing power to handle
// large buffers. On Arduino Mega you shouldn't have to worry much.
char locBuff[250];
char timeBuff[32];

bool netStatus();
void moduleSetup();

void setupFONA()
{
    fona.powerOn(FONA_PWRKEY);
    moduleSetup();

    fona.setFunctionality(1);

    // fona.setNetworkSettings(F("mdata.net.au"));
    fona.setNetworkSettings(F("mdata.net.au"));

    // Perform first-time GPS/data setup if the shield is going to remain on,
    // otherwise these won't be enabled in loop() and it won't work!
    // Enable GPS
    while (!fona.enableGPS(true))
    {
        Serial.println(F("Failed to turn on GPS, retrying..."));
        delay(2000); // Retry every 2s
    }
    Serial.println(F("Turned on GPS!"));

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

void readGPStoJSON(JsonObject &gps_obj)
{
    // Get a fix on location, try every 2s
    // Use the top line if you want to parse UTC time data as well, the line below it if you don't care
    //  while (!fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude, &year, &month, &day, &hour, &minute, &second)) {
    if (!fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude))
    {
        Serial.println(F("Failed to get GPS location..."));
    }

    // Store the results nicely into a JSON document
    gps_obj["lat"] = latitude;
    gps_obj["lon"] = longitude;
    gps_obj["ele"] = altitude;
    // gps_obj["lat"] = -37.78;
    // gps_obj["lon"] = 144.944;
    // gps_obj["ele"] = 10.0;
}

void loopFONA()
{
    // The json document variable
    StaticJsonDocument<200> doc;
    JsonObject root = doc.to<JsonObject>();

    JsonObject sensorDoc = root.createNestedObject("value");
    fona.getBattVoltage(&vBatt);
    fona.getTime(timeBuff,32);
    sensorDoc["nickname"] = String(AIO_BOARDNAME);
    sensorDoc["IMEI"] = imei;
    sensorDoc["battery_mv"] = vBatt;
    sensorDoc["network_time"] = timeBuff;
    sensorDoc["uptime_sec"] = millis()/1000;

    // Read to another subobject for GPS data
    JsonObject gpsDoc = sensorDoc.createNestedObject("gps");
    readGPStoJSON(gpsDoc);

    // Write the JSON output to serial, and the buffer
    serializeJson(doc, locBuff, 250);


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

    // If not already connected, connect to MQTT
    if (!fona.MQTT_connectionStatus())
    {
        // Set up MQTT parameters (see MQTT app note for explanation of parameter values)
        fona.MQTT_setParameter("URL", AIO_SERVER, AIO_SERVERPORT);
        // Set up MQTT username and password if necessary
        fona.MQTT_setParameter("USERNAME", AIO_USERNAME);
        fona.MQTT_setParameter("PASSWORD", AIO_KEY);
        //    fona.MQTTsetParameter("KEEPTIME", 30); // Time to connect to server, 60s by default

        Serial.println(F("Connecting to MQTT broker..."));
        if (!fona.MQTT_connect(true))
        {
            Serial.println(F("Failed to connect to broker!"));
        }
    }
    else
    {
        Serial.println(F("Already connected to MQTT server!"));
    }

    // Now publish all the GPS and temperature data to their respective topics!
    // Parameters for MQTT_publish: Topic, message (0-512 bytes), message length, QoS (0-2), retain (0-1)
    if (!fona.MQTT_publish("leighleighleigh/feeds/bosl-mqtt", locBuff, strlen(locBuff), 1, 0))
        Serial.println(F("Failed to publish!")); // Send GPS location

    // Disconn
    // fona.MQTT_connect(false);
    // fona.openWirelessConnection(false);
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

    type = fona.type();
    Serial.println(F("FONA is OK"));
    Serial.print(F("Found "));
    switch (type)
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
