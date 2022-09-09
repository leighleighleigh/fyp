#include <Arduino.h>
#include "Adafruit_FONA.h" // https://github.com/botletics/SIM7000-LTE-Shield/tree/master/Code
// #include "ArduinoJson.h"
#include <SoftwareSerial.h>
// #include <RTClib.h>

#define SIMCOM_7000
#define FONA_PWRKEY 4
#define FONA_DTR 5 // Connect with solder jumper
#define FONA_TX 9  // Microcontroller RX
#define FONA_RX 8  // Microcontroller TX

SoftwareSerial fonaSerial = SoftwareSerial(FONA_TX, FONA_RX);
Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

/****************************** OTHER STUFF ***************************************/
char imei[16] = {0}; // Use this for device ID
uint8_t type;
uint16_t vBatt;

// char PIN[5] = "1234"; // SIM card PIN

// NOTE: Keep the buffer sizes as small as possible, espeially on
// Arduino Uno which doesn't have much computing power to handle
// large buffers. On Arduino Mega you shouldn't have to worry much.
// char dateTimeFormat[] = "YYMMDD-hh:mm:ss";
char dateTimeFormat[] = "YYYY-MM-DD hh:mm:ss";

char timeBuff[48];
String bootDateTime;

// Create char buffers for the floating point numbers for sprintf
// Make sure these buffers are long enough for your request URL
char URL[128];
char body[128];


bool netStatus();
void moduleSetup();

int replacechar(char *str, char orig, char rep)
{
    char *ix = str;
    int n = 0;
    while ((ix = strchr(ix, orig)) != NULL)
    {
        *ix++ = rep;
        n++;
    }
    return n;
}

void setupFONA()
{
    // Serial.println("powerDown");
    // fona.powerDown();
    Serial.println("powerUp");
    fona.powerOn(FONA_PWRKEY);
    moduleSetup();

    // All features
    Serial.println("setFunc 0");
    fona.setFunctionality(0);
    delay(3000);
    Serial.println("setFunc 1");
    fona.setFunctionality(1);
    delay(3000);

    // fona.setNetworkSettings(F("mdata.net.au"));
    Serial.println("set network");
    fona.setNetworkSettings(F("mdata.net.au"));
    Serial.println("set NTP");
    fona.enableNTPTimeSync(true, F("pool.ntp.org"));

    // Perform first-time GPS/data setup if the shield is going to remain on,
    // otherwise these won't be enabled in loop() and it won't work!
    // Enable GPS
    // while (!fona.enableGPS(false))
    // {
    //     Serial.println(F("Failed to turn on GPS, retrying..."));
    //     delay(2000); // Retry every 2s
    // }
    // Serial.println(F("Turned on GPS!"));

    // turn GPRS off
    if (!fona.enableGPRS(false))
    {
        Serial.println(F("Failed to turn off GPRS..."));
        delay(2000);
    }

    // turn GPRS on
    if (!fona.enableGPRS(true))
    {
        Serial.println(F("Failed to turn on GPRS..."));
        delay(2000);
    }

    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made
    while (!netStatus())
    {
        Serial.println(F("Failed to connect to cell network, retrying..."));
        delay(2000); // Retry every 2s
    }
    Serial.println(F("Connected to cell network!"));

    // READ TIME
    fona.getTime(timeBuff,48);
    // Replace quotes with spaces
    replacechar(timeBuff,'\"',' ');

    Serial.print("Network time: ");
    Serial.println(timeBuff);
    
    // Clean the network time into DateTime format
    String bt = String(timeBuff);
    bt.replace('/','-');
    bt.replace(',',' ');
    unsigned int tsIndex = bt.indexOf('+');
    String btc = bt.substring(0,tsIndex);
    btc.trim();
    bootDateTime = btc;
    Serial.print("Cleaned boot time: ");
    Serial.println(btc);
}

void sendPOST()
{
    // Construct the appropriate URL's and body, depending on request type
    // Use IMEI as device ID for this example
    // The json document variable
    // StaticJsonDocument<128> doc;
    // JsonObject root = doc.to<JsonObject>();

    fona.getBattVoltage(&vBatt);

    // Fill fields in order of appearance in table SCHEMA
    // root["bosl_name"] = String(AIO_BOARDNAME);
    // root["bosl_imei"] = imei;
    // root["bosl_battery_mv"] = vBatt;
    // root["bosl_bootup_timestamp"] = timeBuff;

    // Write the JSON output to serial, and the buffer
    // serializeJson(doc, Serial);
    // serializeJson(doc, body, 200);
    // Serial.print(body);
    // Serial.println();

    // POST request
    // sprintf(URL, "http://dweet.io/dweet/for/%s", imei);
    // sprintf(body, "{\"temp\":%s,\"batt\":%i}", tempBuff, battLevel);
    // Put URL into URL
    const char *token = "nNgr-OJA-K2cNLkfZWQ0B-Xzlrkb9coN";
    // sprintf(URL, "http://demo.thingsboard.io/api/v1/%s/telemetry", token);
    sprintf(URL, "http://cms.leigh.sh/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f");
    // sprintf(URL, "http://cms.leigh.sh/items/column_sensors?access_token=%s",token);

    // First part of JSON data payload
    // Double string escape is needed on the timeBuff entry
    sprintf(body, "{\"data\":\"BW3,%s,%u,20%s\"}", imei, vBatt, bootDateTime.c_str());

    // // Turn JSON keys into CSV here
    // for (JsonPair kv : root) {
    //     Serial.println(kv.key().c_str());
    //     Serial.println(kv.value().as<char*>());
    //     sprintf(body, "%s", kv.value().as<char*>());
    //     Serial.println(body);
    // }

    // // Trailing JSON
    // sprintf(body, "\"}");
    int counter = 0;

    while (counter < 3 && !fona.postData("POST", URL, body, token))
    {
        Serial.println(F("Failed to complete HTTP POST..."));
        counter++;
        delay(1000);
    }

    // if (!fona.postData("POST", URL, body)) // Can also add authorization token parameter!
    // Serial.println(F("Failed to complete HTTP POST..."));
}

void loopFONA()
{

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

    sendPOST();

    // // If not already connected, connect to MQTT
    // if (!fona.MQTT_connectionStatus())
    // {
    //     // Set up MQTT parameters (see MQTT app note for explanation of parameter values)
    //     fona.MQTT_setParameter("URL", AIO_SERVER, AIO_SERVERPORT);
    //     // Set up MQTT username and password if necessary
    //     fona.MQTT_setParameter("USERNAME", AIO_USERNAME);
    //     fona.MQTT_setParameter("PASSWORD", AIO_KEY);
    //     //    fona.MQTTsetParameter("KEEPTIME", 30); // Time to connect to server, 60s by default

    //     Serial.println(F("Connecting to MQTT broker..."));
    //     if (!fona.MQTT_connect(true))
    //     {
    //         Serial.println(F("Failed to connect to broker!"));
    //     }
    // }
    // else
    // {
    //     Serial.println(F("Already connected to MQTT server!"));
    // }

    // // Now publish all the GPS and temperature data to their respective topics!
    // // Parameters for MQTT_publish: Topic, message (0-512 bytes), message length, QoS (0-2), retain (0-1)
    // if (!fona.MQTT_publish("leighleighleigh/feeds/bosl-mqtt", locBuff, strlen(locBuff), 1, 0))
    //     Serial.println(F("Failed to publish!")); // Send GPS location

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
    fonaSerial.begin(9600); // Default SIM7000 shield baud rate

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

    // fona.setBaudrate(9600);

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
