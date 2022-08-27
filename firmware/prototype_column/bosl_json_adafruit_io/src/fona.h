#include <Arduino.h>
#include "Adafruit_FONA.h" // https://github.com/botletics/SIM7000-LTE-Shield/tree/master/Code
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include "ArduinoJson.h"
#include <SoftwareSerial.h>

#define SIMCOM_7000
#define FONA_PWRKEY 4
// #define FONA_RST 7 // unsure if used on BoSL
#define FONA_DTR 5 // Connect with solder jumper
//#define FONA_RI 9 // Need to enable via AT commands
#define FONA_TX 9 // Microcontroller RX
#define FONA_RX 8 // Microcontroller TX

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

/************************* MQTT SETUP *********************************/
// MQTT setup (if you're using it, that is)
// For Adafruit IO:
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883
#define AIO_USERNAME "leighleighleigh"
#define AIO_KEY "ef686e14f1ad4ae1bf7b9cadc5a8213d"

// Setup the FONA MQTT class by passing in the FONA class and MQTT server and login details.
Adafruit_MQTT_FONA mqtt(&fona, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// How many transmission failures in a row we're OK with before reset
uint8_t txfailures = 5;

/****************************** MQTT FEEDS ***************************************/
Adafruit_MQTT_Publish feed_location = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/bosl_mqtt/json");

/****************************** OTHER STUFF ***************************************/
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
char imei[16] = {2}; // Use this for device ID
uint8_t type;
float latitude, longitude, speed_kph, heading, altitude, second;
uint16_t year;
uint8_t month, day, hour, minute;
uint8_t counter = 0;
// char PIN[5] = "1234"; // SIM card PIN

// NOTE: Keep the buffer sizes as small as possible, espeially on
// Arduino Uno which doesn't have much computing power to handle
// large buffers. On Arduino Mega you shouldn't have to worry much.
char locBuff[250];

void moduleSetup();
void MQTT_connect();
void MQTT_publish_checkSuccess(Adafruit_MQTT_Publish &feed, const char *feedContent);
bool netStatus();

void setupFONA()
{
    // pinMode(FONA_RST, OUTPUT);
    // digitalWrite(FONA_RST, HIGH); // Default state

    fona.powerOn(FONA_PWRKEY); // Power on the module
    moduleSetup();             // Establishes first-time serial comm and prints IMEI

    // Unlock SIM card if needed
    // Remember to uncomment the "PIN" variable definition above
    /*
    if (!fona.unlockSIM(PIN)) {
      Serial.println(F("Failed to unlock SIM card"));
    }
    */

    // Set modem to full functionality
    fona.setFunctionality(0); // AT+CFUN=1
    delay(1000);
    fona.setFunctionality(1); // AT+CFUN=1
    fona.setFunctionality(6); // AT+CFUN=1
    delay(10000);
    fona.setFunctionality(1); // AT+CFUN=1

    // fona.setNetworkSettings(F("mdata.net.au"));
    fona.setNetworkSettings(F("telstra.internet"));

    /*
    // Other examples of some things you can set:
    fona.setPreferredMode(38); // Use LTE only, not 2G
    fona.setPreferredLTEMode(1); // Use LTE CAT-M only, not NB-IoT
    fona.setOperatingBand("CAT-M", 12); // AT&T uses band 12
  //  fona.setOperatingBand("CAT-M", 13); // Verizon uses band 13
    fona.enableRTC(true);

    fona.enableSleepMode(true);
    fona.set_eDRX(1, 4, "0010");
    fona.enablePSM(true);

    */

    // Perform first-time GPS/data setup if the shield is going to remain on,
    // otherwise these won't be enabled in loop() and it won't work!
    // Enable GPS
    while (!fona.enableGPS(true))
    {
        Serial.println(F("Failed to turn on GPS, retrying..."));
        delay(2000); // Retry every 2s
    }
    Serial.println(F("Turned on GPS!"));

    // Disable data just to make sure it was actually off so that we can turn it on
    if (!fona.enableGPRS(false))
        Serial.println(F("Failed to disable data!"));

    // Turn on data
    while (!fona.enableGPRS(true))
    {
        Serial.println(F("Failed to enable data, retrying..."));
        delay(2000); // Retry every 2s
    }
    Serial.println(F("Enabled data!"));
}

void readGPStoJSON(JsonObject& gps_obj)
{
    // Get a fix on location, try every 2s
    // Use the top line if you want to parse UTC time data as well, the line below it if you don't care
    //  while (!fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude, &year, &month, &day, &hour, &minute, &second)) {
    while (!fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude))
    {
        Serial.println(F("Failed to get GPS location, retrying..."));
        delay(2000); // Retry every 2s
    }

    // Store the results nicely into a JSON document
    gps_obj["lat"] = latitude;
    gps_obj["lon"] = longitude;
    gps_obj["ele"] = altitude;
}

void loopFONA()
{
    // The json document variable 
    StaticJsonDocument<256> doc;
    JsonObject root = doc.to<JsonObject>();

    // Add timestamp information to the json document
    doc["value"] = millis();


    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made
    while (!netStatus())
    {
        Serial.println(F("Failed to connect to cell network, retrying..."));
        delay(2000); // Retry every 2s
    }
    Serial.println(F("Connected to cell network!"));
    
    // Read GPS
    JsonObject fonaGPS0 = root.createNestedObject("gps");
    readGPStoJSON(fonaGPS0);

    // Write the JSON output to serial, and the buffer
    serializeJson(doc, locBuff);
    serializeJson(doc, Serial);
    Serial.println();

    // Ensure the connection to the MQTT server is alive (this will make the first
    // connection and automatically reconnect when disconnected). See the MQTT_connect
    // function definition further below.
    MQTT_connect();

    // Now publish all the data to different feeds!
    // The MQTT_publish_checkSuccess handles repetitive stuff.
    // You can see the function near the end of this sketch.
    // For the Adafruit IO dashboard map we send the combined lat/long buffer
    MQTT_publish_checkSuccess(feed_location, locBuff);

}

void moduleSetup()
{
    // SIM7000 takes about 3s to turn on and SIM7500 takes about 15s
    // Press Arduino reset button if the module is still turning on and the board doesn't find it.
    // When the module is on it should communicate right after pressing reset

    // Software serial:
    fonaSS.begin(115200); // Default SIM7000 shield baud rate

    Serial.println(F("Configuring to 9600 baud"));
    fonaSS.println("AT+IPR=9600"); // Set baud rate
    delay(100);                    // Short pause to let the command run
    fonaSS.begin(9600);
    if (!fona.begin(fonaSS))
    {
        Serial.println(F("Couldn't find FONA"));
        while (1)
            ; // Don't proceed if it couldn't find the device
    }

    // Hardware serial:
    /*
    fonaSerial->begin(115200); // Default SIM7000 baud rate

    if (! fona.begin(*fonaSerial)) {
      DEBUG_PRINTLN(F("Couldn't find SIM7000"));
    }
    */

    // The commented block of code below is an alternative that will find the module at 115200
    // Then switch it to 9600 without having to wait for the module to turn on and manually
    // press the reset button in order to establish communication. However, once the baud is set
    // this method will be much slower.
    /*
    fonaSerial->begin(115200); // Default LTE shield baud rate
    fona.begin(*fonaSerial); // Don't use if statement because an OK reply could be sent incorrectly at 115200 baud

    Serial.println(F("Configuring to 9600 baud"));
    fona.setBaudrate(9600); // Set to 9600 baud
    fonaSerial->begin(9600);
    if (!fona.begin(*fonaSerial)) {
      Serial.println(F("Couldn't find modem"));
      while(1); // Don't proceed if it couldn't find the device
    }
    */

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

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect()
{
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected())
    {
        return;
    }

    Serial.println("Connecting to MQTT... ");

    while ((ret = mqtt.connect()) != 0)
    { // connect will return 0 for connected
        Serial.println(mqtt.connectErrorString(ret));
        Serial.println("Retrying MQTT connection in 5 seconds...");
        mqtt.disconnect();
        delay(5000); // wait 5 seconds
    }
    Serial.println("MQTT Connected!");
}

void MQTT_publish_checkSuccess(Adafruit_MQTT_Publish &feed, const char *feedContent)
{
    Serial.println(F("Sending data..."));
    if (!feed.publish(feedContent))
    {
        Serial.println(F("Failed"));
        txfailures++;
    }
    else
    {
        Serial.println(F("OK!"));
        txfailures = 0;
    }
}