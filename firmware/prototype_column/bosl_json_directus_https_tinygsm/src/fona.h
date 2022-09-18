#include <Arduino.h>

#define TINY_GSM_MODEM_SIM7000SSL
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

// #include "Adafruit_FONA.h" // https://github.com/botletics/SIM7000-LTE-Shield/tree/master/Code
// #include "ArduinoJson.h"
#include <SoftwareSerial.h>
// #include <RTClib.h>

const char boardName[] = "BW3";
const char token[] = "nNgr-OJA-K2cNLkfZWQ0B-Xzlrkb9coN";
const char URL[] = "***REMOVED***/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f";
const char URLCONTENT[] = "/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f";
// const char URLBASE[] = "https://***REMOVED***";
const char URLBASE[] = "***REMOVED***";
// const char URI[] = "/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f";

#define FONA_PWRKEY 4
#define FONA_DTR 5 // Connect with solder jumper
#define FONA_TX 9  // Microcontroller RX
#define FONA_RX 8  // Microcontroller TX
#define SerialMon Serial

SoftwareSerial fonaSerial = SoftwareSerial(FONA_TX, FONA_RX);
TinyGsm modem(fonaSerial);
TinyGsmClientSecure fona(modem);
HttpClient http(fona, URLBASE, 443);

// Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

/****************************** OTHER STUFF ***************************************/
String imei;
// char imei[16] = {0}; // Use this for device ID
// uint8_t type;
uint16_t vBatt;

// char PIN[5] = "1234"; // SIM card PIN

// NOTE: Keep the buffer sizes as small as possible, espeially on
// Arduino Uno which doesn't have much computing power to handle
// large buffers. On Arduino Mega you shouldn't have to worry much.
// char dateTimeFormat[] = "YYMMDD-hh:mm:ss";
char dateTimeFormat[] = "YYYY-MM-DD hh:mm:ss";

char timeBuff[48];
String bootDateTime;

// GLOBAL SENSOR READINGS
int rawTemp, rawSoil;
int lower_rawA, lower_rawB;
float lower_rawAverage, lower_sensorResistance;
int upper_rawA, upper_rawB;
float upper_rawAverage, upper_sensorResistance;

bool netStatus();
boolean moduleSetup();

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

void simOn();

bool hasBooted = false;

boolean setupFONA()
{
    Serial.println("powerUp");

    // fona.powerOn(FONA_PWRKEY);
    simOn();
    delay(6000);

    boolean foundModule = moduleSetup();
    if (!foundModule)
    {
        return false;
    }

    // fona.sendCheckReply(F("ATE0"),F("OK"),500);
    // fona.sendCheckReply(F("AT&W0"),F("OK"),500);

    // Serial.println("setFunc 0");
    // modem.setPhoneFunctionality()
    // fona.setFunctionality(0);
    // delay(3000);
    // // Reset if needed
    // Serial.println("setFunc 1");
    // fona.setFunctionality(1);
    // delay(3000);

    // Serial.println("setFunc 6...");
    // fona.setFunctionality(6);
    // delay(10000);

    // fona.sendCheckReply(F("ATE0"),F("OK"),500);
    // fona.sendCheckReply(F("ATE0"),F("OK"),500);

    // fona.sendCheckReply(F("ATE0"),F("OK"),500);
    // fona.sendCheckReply(F("ATE0"),F("OK"),500);

    // fona.setFunctionality(1);
    // delay(3000);

    // // Turn off echo
    // fona.sendCheckReply(F("ATE0"),F("OK"),1000);
    // Serial.println("setFunc 1");
    // fona.setFunctionality(1);
    // delay(3000);

    // All features
    // fona.setNetworkSettings(F("mdata.net.au"));
    Serial.println("set network");
    // fona.setNetworkSettings(F("mdata.net.au"));
    modem.gprsConnect("mdata.net.au");

    // Improve SNR time
    // fona.sendCheckReply(F("AT+CNBS=1"),F("OK"),1000U);

    // fona.setHTTPSRedirect(true);
    /* MODE SELECT AND OPERATING BAND MUST OCCUR AFTER NETWORK SETTINGS CALL */
    // fona.setPreferredMode(51); // GSM+LTE ONLY
    // fona.setPreferredLTEMode(1); // CAT-M ONLY
    // These are all the LTE bands telstra support
    // fona.setOperatingBand("CAT-M", 1);
    // fona.setOperatingBand("CAT-M", 3);
    // fona.setOperatingBand("CAT-M", 7); // UNSUPPORTED BY SIM7000G
    // fona.setOperatingBand("CAT-M", 8);
    // fona.setOperatingBand("CAT-M", 28);

    // Serial.println("set NTP");
    // fona.enableNTPTimeSync(true, F("pool.ntp.org"));
    modem.NTPServerSync();

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
    // for (int i = 0; i < 3; i++)
    // {
    // if (!fona.enableGPRS(false))
    // {
    // Serial.println(F("Failed to turn off GPRS..."));
    // delay(2000);
    // }
    // }

    // turn GPRS on
    // for (int i = 0; i < 3; i++)
    // {
    // while (!modem.isGprsConnected())
    // {
    //     Serial.println(F("GPRS not yet connected..."));
    //     delay(2000);
    // }
    // }

    // Connect to cell network and verify connection
    // If unsuccessful, keep retrying every 2s until a connection is made
    // while (!netStatus())
    // {
    //     Serial.println(F("Failed to connect to cell network, retrying..."));
    //     delay(2000); // Retry every 2s
    // }
    // Serial.println(F("Connected to cell network!"));

    SerialMon.print("Waiting for network...");
    while(!modem.waitForNetwork()) {
        SerialMon.println(" fail");
        delay(10000);
    }
    SerialMon.println(" success");

    if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }

    // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.print("mdata.net.au");
    while (!modem.gprsConnect("mdata.net.au")) {
        SerialMon.println(" fail");
        delay(10000);
    }
    SerialMon.println(" success");

    if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }

    // READ TIME
    if (!hasBooted)
    {
        // Clean the network time into DateTime format
        String bt = modem.getGSMDateTime(DATE_FULL);
        Serial.print("Time: ");
        Serial.println(bt);

        bt.replace('/', '-');
        bt.replace(',', ' ');
        unsigned int tsIndex = bt.indexOf('+');
        String btc = bt.substring(0, tsIndex);
        btc.trim();
        bootDateTime = btc;
        Serial.print("Cleaned boot time: ");
        Serial.println(btc);
        hasBooted = true;
    }

    return true;
}

void sendPOST()
{
    // Create char buffers for the floating point numbers for sprintf
    // Make sure these buffers are long enough for your request URL
    // char body[180];
    // uint32_t bodylen = 0;

    // char URIARR[180];

    // Construct the appropriate URL's and body, depending on request type
    vBatt = modem.getBattVoltage();

    // POST request
    // sprintf(URL, "http://dweet.io/dweet/for/%s", imei);
    // sprintf(body, "{\"temp\":%s,\"batt\":%i}", tempBuff, battLevel);
    // Put URL into URL
    // sprintf(URL, "http://demo.thingsboard.io/api/v1/%s/telemetry", token);
    // sprintf(URL, "http://***REMOVED***/items/column_sensors?access_token=%s",token);

    // First part of JSON data payload
    // Double string escape is needed on the timeBuff entry
    // bodylen = sprintf(body, "{\"data\":\"%s,%s,%u,20%s,%d,%d,%s,%d,%d,%d,%s,%d,0.0,0.0,%s,0.0,0.0,%s\"}\0", boardName, imei, vBatt, bootDateTime.c_str(), upper_rawA, upper_rawB, String(upper_rawAverage).c_str(), (uint16_t)upper_sensorResistance, lower_rawA, lower_rawB, String(lower_rawAverage).c_str(), (uint16_t)lower_sensorResistance, String(rawTemp).c_str(), String(rawSoil).c_str());

    // sprintf(URIARR, "%s", URI);

    // Start HTTPS connection

    // Connect to server
    // If https:// is used, #define SSL_FONA 1 in Adafruit_FONA.h
    // if (! fona.HTTP_connect(URLBASE)) {
    // Serial.println(F("Failed to connect to server..."));
    // }else{
    // Serial.print("Connected to ");
    // Serial.println(URLBASE);
    // }

    // Add headers as needed
    // fona.HTTP_addHeader("User-Agent", "SIM7000", 7);
    // fona.HTTP_addHeader("Cache-control", "no-cache", 8);
    // fona.HTTP_addHeader("Connection", "keep-alive", 10);
    // fona.HTTP_addHeader("Accept", "*/*", 3);
    // fona.HTTP_addHeader("Content-Type", "application/json", 16);

    // fona.HTTP_POST(URI, body, strlen(body));

    // int counter = 0;
    // // fona.connect(URLBASE,443);
    // http.get(URLCONTENT);

    // while(!fona.connected())
    // {
    //     Serial.println("Not connected...");
    //     delay(1000);
    // }

    // Serial.println("Connected!");
    SerialMon.print(F("Performing HTTPS GET request... "));
    http.connectionKeepAlive(); // Currently, this is needed for HTTPS
    int err = http.get(URLCONTENT);
    if (err != 0)
    {
        SerialMon.println(F("failed to connect"));
        delay(10000);
        return;
    }

    int status = http.responseStatusCode();
    SerialMon.print(F("Response status code: "));
    SerialMon.println(status);
    if (!status)
    {
        delay(10000);
        return;
    }

    SerialMon.println(F("Response Headers:"));
    while (http.headerAvailable())
    {
        String headerName = http.readHeaderName();
        String headerValue = http.readHeaderValue();
        SerialMon.println("    " + headerName + " : " + headerValue);
    }

    int length = http.contentLength();
    if (length >= 0)
    {
        SerialMon.print(F("Content length is: "));
        SerialMon.println(length);
    }
    if (http.isResponseChunked())
    {
        SerialMon.println(F("The response is chunked"));
    }

    String body = http.responseBody();
    SerialMon.println(F("Response:"));
    SerialMon.println(body);

    SerialMon.print(F("Body length is: "));
    SerialMon.println(body.length());

    // Shutdown

    http.stop();
    SerialMon.println(F("Server disconnected"));

    // while (counter < 3 && !fona.postData("POST", URL, body, token, bodylen))
    // {
    //     Serial.println(F("Failed to complete HTTP POST..."));
    //     counter++;
    //     delay(1000);
    // }

    // if (!fona.postData("POST", URL, body)) // Can also add authorization token parameter!
    // Serial.println(F("Failed to complete HTTP POST..."));
}

void loopFONA()
{

    // Open wireless connection if not already activated
    // if (!fona.wirelessConnStatus())
    // {
    //     while (!fona.openWirelessConnection(true))
    //     {
    //         Serial.println(F("Failed to enable connection, retrying..."));
    //         delay(2000); // Retry every 2s
    //     }
    //     Serial.println(F("Enabled data!"));
    // }
    // else
    // {
    //     Serial.println(F("Data already enabled!"));
    // }

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

void simOn()
{
    // powers on SIM7000

    // do check for if sim is on
    pinMode(FONA_PWRKEY, OUTPUT);
    digitalWrite(FONA_PWRKEY, LOW);
    delay(1000); // For SIM7000
    digitalWrite(FONA_PWRKEY, HIGH);
    delay(4000);
}

void simOff()
{
    // powers off SIM7000
    digitalWrite(FONA_PWRKEY, LOW);
    delay(1200); // For SIM7000
    digitalWrite(FONA_PWRKEY, HIGH);
    delay(2000);
}

void shutdownFONA()
{
    // Serial.println("Shutdown... (DISABLED)");
    // fona.powerDown();
    // modem.poweroff();
    simOff();
    // fonaSerial.flush();
}

boolean tryBegin(uint32_t baud)
{
    Serial.print("Opening SerialAT at ");
    Serial.print(baud);
    Serial.println(" baud.");

    fonaSerial.begin(baud); // Default SIM7000 shield baud rate

    // Serial.println(F("Configuring to 9600 baud"));
    // fonaSerial.println("AT+IPR=9600"); // Set baud rate
    // delay(100);                        // Short pause to let the command run
    // fonaSerial.begin(9600);

    if (!modem.init())
    {
        Serial.println(F("Couldn't find FONA"));
        fonaSerial.end();
        return false;
    }

    Serial.println(F("Found FONA!"));
    return true;
}

boolean moduleSetup()
{
    // SIM7000 takes about 3s to turn on and SIM7500 takes about 15s
    // Press Arduino reset button if the module is still turning on and the board doesn't find it.
    // When the module is on it should communicate right after pressing reset

    // Try at 9600
    boolean found = tryBegin(9600);

    if (!found)
    {
        // else try at 115200, then set to 9600
        found = tryBegin(115200);

        if (found)
        {
            modem.setBaud(9600);
            // Now re-start at 9600
            found = tryBegin(9600);
        }
        else
        {
            Serial.println("COULDNT FIND FONA!");
            return false;
        }
    }

    // Print module IMEI number.
    imei = modem.getIMEI();

    return true;
}

bool netStatus()
{
    RegStatus n = modem.getRegistrationStatus();

    Serial.print(F("Network status "));
    Serial.print(n);
    Serial.print(F(": "));
    if (n == REG_UNREGISTERED)
        Serial.println(F("Not registered"));
    if (n == REG_OK_HOME)
        Serial.println(F("Registered (home)"));
    if (n == REG_SEARCHING)
        Serial.println(F("Not registered (searching)"));
    if (n == REG_DENIED)
        Serial.println(F("Denied"));
    if (n == REG_UNKNOWN)
        Serial.println(F("Unknown"));
    if (n == REG_OK_ROAMING)
        Serial.println(F("Registered roaming"));

    if (!(n == 1 || n == 5))
        return false;
    else
        return true;
}
