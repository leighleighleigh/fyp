#include <Arduino.h>
#include <schema.h>
#include <SoftwareSerial.h>

#define TINY_GSM_MODEM_SIM7000SSL

#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif

// set GSM PIN, if any
#define GSM_PIN ""

#define SIM_PWRKEY 4
#define SIM_DTR 5 // Connect with solder jumper
#define BOSL_RX 9  // Microcontroller RX
#define BOSL_TX 8  // Microcontroller TX

#define SerialMon Serial
SoftwareSerial SerialAT(BOSL_RX, BOSL_TX); // RX, TX

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define TINY_GSM_YIELD() { delay(2); }

// set GSM PIN, if any
#define GSM_PIN ""

// flag to force SSL client authentication, if needed
// #define TINY_GSM_SSL_CLIENT_AUTHENTICATION

// Your GPRS credentials, if any
const char apn[] = "mdata.net.au";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details
const char server[] = "mc.leigh.sh";
const char resource[] = "/index.html";
const int port = 443;
// Get this fingerprint with the script in the tools folder!
const char* fingerprint = "EB D0 96 C6 93 1F A3 8A E4 B1 D7 EE C5 A6 65 DE 10 17 6B F5";

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

TinyGsm modem(SerialAT);
TinyGsmClientSecure client(modem);
HttpClient http(client, server, port);

void simOn()
{
    // powers on SIM7000
    // do check for if sim is on
    pinMode(SIM_PWRKEY, OUTPUT);
    pinMode(BOSL_TX, OUTPUT);
    digitalWrite(BOSL_TX, HIGH);
    pinMode(BOSL_RX, INPUT_PULLUP);
    digitalWrite(SIM_PWRKEY, LOW);
    delay(1000); // For SIM7000
    digitalWrite(SIM_PWRKEY, HIGH);
    delay(4000);
}

void simOff()
{
    // powers off SIM7000
    //  TX / RX pins off to save power
    digitalWrite(BOSL_TX, LOW);
    digitalWrite(BOSL_RX, LOW);
    digitalWrite(SIM_PWRKEY, LOW);
    delay(1200); // For SIM7000
    digitalWrite(SIM_PWRKEY, HIGH);
    delay(2000);
}

void setupFONA()
{
    // Set console baud rate
    SerialMon.begin(115200);
    delay(10);

    // !!!!!!!!!!!
    // Set your reset, enable, power pins here
    // !!!!!!!!!!!

    simOff();
    SerialMon.println("Wait...");

    // Set GSM module baud rate
    SerialAT.begin(9600);
    simOn();
    delay(6000);

    // Restart takes quite some time
    // To skip it, call init() instead of restart()
    SerialMon.println("Initializing modem...");
    modem.restart();
    // modem.init();

    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem Info: ");
    SerialMon.println(modemInfo);

    // Unlock your SIM card with a PIN if needed
    if (GSM_PIN && modem.getSimStatus() != 3)
    {
        modem.simUnlock(GSM_PIN);
    }
}

void loopFONA()
{
    SerialMon.print("Waiting for network...");
    if (!modem.waitForNetwork())
    {
        SerialMon.println(" fail");
        delay(10000);
        return;
    }
    SerialMon.println(" success");

    if (modem.isNetworkConnected())
    {
        SerialMon.println("Network connected");
    }

    // Update time for proper SSL communications
    modem.NTPServerSync("pool.ntp.org",3U);

    // Update SSL params
    // client.setCertificate();

    // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to "));
    SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass))
    {
        SerialMon.println(" fail");
        delay(10000);
        return;
    }
    SerialMon.println(" success");

    if (modem.isGprsConnected())
    {
        SerialMon.println("GPRS connected");
    }



    SerialMon.print(F("Performing HTTPS GET request... "));
    http.connectionKeepAlive(); // Currently, this is needed for HTTPS
    // http.sendHeader(F("Content-Type"), F("application/json"));

    int err = http.get(resource);
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

    modem.gprsDisconnect();

    // Do nothing forevermore
    while (true)
    {
        delay(1000);
    }
}
// SoftwareSerial fonaSerial = SoftwareSerial(BOSL_RX, BOSL_TX);
// Adafruit_FONA_LTE fona = Adafruit_FONA_LTE();

// // #define DIRECTUS_URL "http://cms.leigh.sh/items/column_sensors"
// #define DIRECTUS_URL "https://cms.leigh.sh/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f"
// #define DIRECTUS_TOKEN "PXzLIINzWiEsd13j0eMloz2QbtB7LzAs"

// /****************************** OTHER STUFF ***************************************/
// char imei[16] = {0};
// uint8_t fonaType;
// String body;

// bool netStatus();
// void moduleSetup();

// void setupFONA()
// {

//     fona.powerOn(SIM_PWRKEY);
//     moduleSetup();

//     fona.setFunctionality(1);

//     // fona.setNetworkSettings(F("mdata.net.au"));
//     fona.setNetworkSettings(F("mdata.net.au"));
//     fona.setHTTPSRedirect(true);

//     // Connect to cell network and verify connection
//     // If unsuccessful, keep retrying every 2s until a connection is made
//     while (!netStatus())
//     {
//         Serial.println(F("Failed to connect to cell network, retrying..."));
//         delay(5000); // Retry every 2s
//     }
//     Serial.println(F("Connected to cell network!"));
// }

// void loopFONA()
// {
//     uint16_t vbatt;
//     fona.getBattVoltage(&vbatt);
//     char body[100];
//     // sprintf(body, "%s", "{\"bosl_name\":\"BW3\"}"));
//     // sprintf(body, column_data_schema, "BW3", (char *)imei, vbatt, "time", 1,2,"3.123",4,5,6,"7.123",8,"9.1","9.2","9.3","9.4","9.5","9.6");
//     sprintf(body, column_data_schema, "BW3", (char * )imei, vbatt);
//     // doc["bosl_name"] = "BW3";
//     // doc["bosl_imei"] = String(imei);
//     // doc["bosl_battery_mv"] = vbatt;

//     // // Top sensors
//     // doc["chmln_top_raw_a"] = 300;
//     // doc["chmln_top_raw_b"] = 250;
//     // doc["chmln_top_raw_avg"] = 6000;
//     // doc["chmln_top_resistance"] = 200;
//     // doc["smt100_top_vwc"] = 10.5;
//     // doc["smt100_top_temperature"] = 24;

//     // // Bottom sensors
//     // doc["chmln_bot_raw_a"] = 300;
//     // doc["chmln_bot_raw_b"] = 250;
//     // doc["chmln_bot_raw_avg"] = 6000;
//     // doc["chmln_bot_resistance"] = 22;

//     // doc["ds18b20_bot_temperature"] = 22;

//     // Open wireless connection if not already activated
//     if (!fona.wirelessConnStatus())
//     {
//         while (!fona.openWirelessConnection(true))
//         {
//             Serial.println(F("Failed to enable connection, retrying..."));
//             delay(2000); // Retry every 2s
//         }
//         Serial.println(F("Enabled data!"));
//     }
//     else
//     {
//         Serial.println(F("Data already enabled!"));
//     }

//     char URL[150];
//     char TOKENSTR[64];

//     sprintf(TOKENSTR, "%s", DIRECTUS_TOKEN);
//     sprintf(URL, "%s", DIRECTUS_URL);

//     // serializeJson(doc, Serial);
//     Serial.print("body length: ");
//     Serial.print(strlen(body));
//     Serial.println();
//     Serial.println("body: ");
//     Serial.print(body);
//     Serial.println();

//     fona.addRootCA(root_ca);
//     fona.HTTP_ssl(true);
//     // if (! fona.HTTP_connect("https://cms.leigh.sh")) {
//         // Serial.println(F("Failed to connect to server..."));
//         // return;
//     // }

//     // sprintf(URL, "%s", "/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f"); // Format URI

//     // Format JSON body for POST request
//     // Example JSON body: "{\"temp\":\"22.3\",\"batt\":\"3800\"}"
//     // fona.HTTP_addHeader("Content-Type", "application/json", 16);
//     // fona.HTTP_addHeader("Authorization", "Bearer PXzLIINzWiEsd13j0eMloz2QbtB7LzAs", 48);
//     // fona.HTTP_POST(URL, body, strlen(body));
//     fona.postData("POST", URL, body, TOKENSTR);

// }

// void moduleSetup()
// {
//     // SIM7000 takes about 3s to turn on and SIM7500 takes about 15s
//     // Press Arduino reset button if the module is still turning on and the board doesn't find it.
//     // When the module is on it should communicate right after pressing reset

//     // Software serial:
//     fonaSerial.begin(115200); // Default SIM7000 shield baud rate

//     Serial.println(F("Configuring to 9600 baud"));
//     fonaSerial.println("AT+IPR=9600"); // Set baud rate
//     delay(100);                        // Short pause to let the command run
//     fonaSerial.begin(9600);
//     if (!fona.begin(fonaSerial))
//     {
//         Serial.println(F("Couldn't find FONA"));
//         while (1)
//             ; // Don't proceed if it couldn't find the device
//     }

//     fonaType = fona.type();
//     Serial.println(F("FONA is OK"));
//     Serial.print(F("Found "));
//     switch (fonaType)
//     {
//     case SIM800L:
//         Serial.println(F("SIM800L"));
//         break;
//     case SIM800H:
//         Serial.println(F("SIM800H"));
//         break;
//     case SIM808_V1:
//         Serial.println(F("SIM808 (v1)"));
//         break;
//     case SIM808_V2:
//         Serial.println(F("SIM808 (v2)"));
//         break;
//     case SIM5320A:
//         Serial.println(F("SIM5320A (American)"));
//         break;
//     case SIM5320E:
//         Serial.println(F("SIM5320E (European)"));
//         break;
//     case SIM7000:
//         Serial.println(F("SIM7000"));
//         break;
//     case SIM7070:
//         Serial.println(F("SIM7070"));
//         break;
//     case SIM7500:
//         Serial.println(F("SIM7500"));
//         break;
//     case SIM7600:
//         Serial.println(F("SIM7600"));
//         break;
//     default:
//         Serial.println(F("???"));
//         break;
//     }

//     // Print module IMEI number.
//     uint8_t imeiLen = fona.getIMEI(imei);
//     if (imeiLen > 0)
//     {
//         Serial.print("Module IMEI: ");
//         Serial.println(imei);
//     }
// }

// bool netStatus()
// {
//     int n = fona.getNetworkStatus();

//     Serial.print(F("Network status "));
//     Serial.print(n);
//     Serial.print(F(": "));
//     if (n == 0)
//         Serial.println(F("Not registered"));
//     if (n == 1)
//         Serial.println(F("Registered (home)"));
//     if (n == 2)
//         Serial.println(F("Not registered (searching)"));
//     if (n == 3)
//         Serial.println(F("Denied"));
//     if (n == 4)
//         Serial.println(F("Unknown"));
//     if (n == 5)
//         Serial.println(F("Registered roaming"));

//     if (!(n == 1 || n == 5))
//         return false;
//     else
//         return true;
// }
