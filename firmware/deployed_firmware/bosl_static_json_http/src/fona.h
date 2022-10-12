#include <Arduino.h>
#define SIMCOM_7000
// #include "ArduinoJson.h"
// #include <RTClib.h>
#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>

#ifndef BOARDNAME
#define BOARDNAME "UNKOWN"
#endif

#ifndef TOKEN
#define TOKEN ""
// #define TOKEN "nNgr-OJA-K2cNLkfZWQ0B-Xzlrkb9coN"
#endif

#ifndef SERVER
#define SERVER "bosl.com.au/IoT/wsudwatch/FYP_SGI/log.php"
// #define SERVER "cms.leigh.sh/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f"
#endif

const char boardName[] = BOARDNAME;
const char token[] = TOKEN;
const char URL[] = SERVER;

// What baud do we try immediately after boot, or set the module to after trying default.
#define FONA_BAUD 9600
// What baud to we assume is default
#define FONA_DEFAULT_BAUD 115200

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
char dateTimeFormat[] = "YYYY-MM-DD hh:mm:ss";

char timeBuff[48];
String bootDateTime;

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

// void simOn();
bool hasBooted = false;

boolean setupFONA()
{
    // Serial.println("powerUp");
    fona.powerOn(FONA_PWRKEY);
    delay(6000);

    boolean foundModule = moduleSetup();
    if (!foundModule)
    {
        return false;
    }

    // Get the battery input! (This is the SIM7000G ADC)
    fona.getBattVoltage(&vBatt);
    // Serial.print("vBatt: ");
    // Serial.print(vBatt);
    // Serial.println("mV");

    // Serial.println("setFunc 0");
    fona.setFunctionality(0);
    delay(3000);
    // Reset if needed
    // Serial.println("setFunc 1");
    fona.setFunctionality(1);
    delay(3000);

    if (!hasBooted)
    {
        Serial.println("setFunc 6...");
        fona.setFunctionality(6);
        delay(30000);

        fona.sendCheckReply(F("ATE0"), F("OK"), 500);
        fona.sendCheckReply(F("ATE0"), F("OK"), 500);
        fona.sendCheckReply(F("ATE0"), F("OK"), 500);
        fona.sendCheckReply(F("ATE0"), F("OK"), 500);

        fona.setFunctionality(1);
        delay(3000);
    }

    // // Turn off echo
    // fona.sendCheckReply(F("ATE0"),F("OK"),1000);
    // Serial.println("setFunc 1");
    // fona.setFunctionality(1);
    // delay(3000);

    // All features
    // fona.setNetworkSettings(F("mdata.net.au"));
    // Serial.println("set network");
    fona.setNetworkSettings(F("mdata.net.au"));

    // Improve SNR time
    fona.sendCheckReply(F("AT+CNBS=1"), F("OK"), 1000U);

    fona.setHTTPSRedirect(true);
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
    while (!fona.enableGPRS(true))
    {
        if (millis() > 30000 && !hasBooted)
        {
            // We tried to do this for 30 seconds, on our first boot - lets just skip this step and continue.
            Serial.println(F("Failed GPRS setting for 30 seconds on first boot, skipping..."));
            delay(1000);
            break;
        }
        Serial.println(F("Failed to turn on GPRS..."));
        delay(2000);
        if (!fona.enableGPRS(false))
        {
            Serial.println(F("Failed to turn off GPRS..."));
            delay(2000);
        }
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
    if (!hasBooted)
    {
        fona.getTime(timeBuff, 48);
        // Replace quotes with spaces
        replacechar(timeBuff, '\"', ' ');

        Serial.print("Network time: ");
        Serial.println(timeBuff);

        // Clean the network time into DateTime format
        String bt = String(timeBuff);
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

    return true;
}

void shutdownFONA()
{
    // Doesn't toggle the power key, which prevents this command waking up a sleeping SIM.
    Serial.print("Powering down modem...");
    fona.powerDown();
    // Wait for 2 seconds to be sure - max response time is 1 second
    delay(2000);
    Serial.println("Done");
    // simOff();
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

    if (!fona.begin(fonaSerial))
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
    boolean found = tryBegin(FONA_BAUD);

    if (!found)
    {
        // else try at 115200, then set to 9600
        found = tryBegin(FONA_DEFAULT_BAUD);

        if (found)
        {
            fona.setBaudrate(FONA_BAUD);
            // Now re-start at 9600
            found = tryBegin(FONA_BAUD);
        }
        else
        {
            Serial.println("COULDNT FIND FONA!");
            return false;
        }
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

    return true;
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
