#include <Arduino.h>

#define SIMCOM_7000
#define SSL_FONA 1

#include "Adafruit_FONA.h" // https://github.com/botletics/SIM7000-LTE-Shield/tree/master/Code
// #include "ArduinoJson.h"
#include <SoftwareSerial.h>
// #include <RTClib.h>

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


// GLOBAL SENSOR READINGS
unsigned int rawTemp, rawSoil;
unsigned int lower_rawA, lower_rawB;
float lower_rawAverage, lower_sensorResistance;
unsigned int upper_rawA, upper_rawB;
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

// void simOn();

bool hasBooted = false;

boolean setupFONA()
{
    Serial.println("powerUp");
    fona.powerOn(FONA_PWRKEY);
    delay(6000);

    boolean foundModule = moduleSetup();
    if(!foundModule)
    {
        return false;
    }

    // fona.sendCheckReply(F("AT+CFUN=6"),F("OK"),6000U);
    // delay(10000);
    // fona.sendCheckReply(F("ATE0"),F("OK"),6000U);
    // fona.sendCheckReply(F("AT+CFUN=0"),F("OK"),6000U);
    // delay(1000);
    // fona.sendCheckReply(F("AT+CNCFG=0,\"mdata.net.au\""),F("OK"),6000U);
    // delay(1000);
    // fona.sendCheckReply(F("AT+CGDCONT=1,\"IP\",\"mdata.net.au\""),F("OK"),6000U);
    // delay(1000);
    // fona.sendCheckReply(F("AT+CFUN=1"),F("OK"),6000U);
    // delay(1000);
    // fona.sendCheckReply(F("AT+CPIN?"),F("OK"),6000U);
    // fona.sendCheckReply(F("AT+CGATT?"),F("OK"),6000U);
    // fona.sendCheckReply(F("AT+CGNAPN"),F("OK"),6000U);
    // fona.sendCheckReply(F("AT+CNACT=1"),F("OK"),6000U);
    // fona.sendCheckReply(F("AT+CNACT?"),F("OK"),6000U);
    // delay(3000);
    // fona.sendCheckReply(F(""),F("OK"),1000U);

    // fona.sendCheckReply(F("ATE0"),F("OK"),500);
    // fona.sendCheckReply(F("AT&W0"),F("OK"),500);

    Serial.println("setFunc 0");
    fona.setFunctionality(0);
    delay(1000);
    // Reset if needed
    // Serial.println("setFunc 1");
    // fona.setFunctionality(1);
    // delay(3000);

    Serial.println("setFunc 6...");
    fona.setFunctionality(6);
    delay(10000);

    fona.sendCheckReply(F("ATE0"),F("OK"),500);
    // fona.sendCheckReply(F("ATE0"),F("OK"),500);

    // fona.sendCheckReply(F("ATE0"),F("OK"),500);
    // fona.sendCheckReply(F("ATE0"),F("OK"),500);

    fona.setFunctionality(1);
    delay(1000);

    // // Turn off echo
    // fona.sendCheckReply(F("ATE0"),F("OK"),1000);
    // Serial.println("setFunc 1");
    // fona.setFunctionality(1);
    // delay(3000);

    // All features
    fona.setNetworkSettings(F("mdata.net.au"));
    // Serial.println("set network");
    // fona.setNetworkSettings(F("mdata.net.au"));

    // Improve SNR time
    fona.sendCheckReply(F("AT+CNBS=1"),F("OK"),1000U);

    // fona.setHTTPSRedirect(true);
    /* MODE SELECT AND OPERATING BAND MUST OCCUR AFTER NETWORK SETTINGS CALL */
    // fona.setPreferredMode(50); // GSM+LTE ONLY
    // fona.setPreferredLTEMode(1); // CAT-M ONLY
    // These are all the LTE bands telstra support
    // fona.setOperatingBand("CAT-M", 1);
    // fona.setOperatingBand("CAT-M", 3);
    // fona.setOperatingBand("CAT-M", 7); // UNSUPPORTED BY SIM7000G
    // fona.setOperatingBand("CAT-M", 8);
    // fona.setOperatingBand("CAT-M", 28);

    // Serial.println("set NTP");
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
    while (!fona.enableGPRS(true))
    {
        Serial.println(F("Failed to turn on GPRS..."));
        delay(2000);
        if (!fona.enableGPRS(false))
        {
            Serial.println(F("Failed to turn off GPRS..."));
            delay(2000);
        }
    }
    // }

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

    return true;
}

const char token[] = "nNgr-OJA-K2cNLkfZWQ0B-Xzlrkb9coN";
const char URL[] = "cms.leigh.sh/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f";
const char URLBASE[] = "https://cms.leigh.sh";
const char URI[] = "/flows/trigger/625c3333-48bf-4397-a5a6-a0d72e204b6f";
// const char URI[] = "/post";

void sendPOST()
{
    // Create char buffers for the floating point numbers for sprintf
    // Make sure these buffers are long enough for your request URL
    char body[180];
    uint32_t bodylen = 0;

    // Construct the appropriate URL's and body, depending on request type
    fona.getBattVoltage(&vBatt);

    bodylen = sprintf(body, "{\"data\":\"BW3,%s,%u,20%s,%u,%u,%s,%u,%u,%u,%s,%u,0.0,0.0,%s,0.0,0.0,%s\"}\0", imei, vBatt, bootDateTime.c_str(), upper_rawA, upper_rawB, String(upper_rawAverage).c_str(), (uint16_t)upper_sensorResistance, lower_rawA, lower_rawB, String(lower_rawAverage).c_str(), (uint16_t)lower_sensorResistance, String(rawTemp).c_str(), String(rawSoil).c_str());

    Serial.print("NTP stat: ");
    Serial.println(fona.getNTPstatus());
    // fona.HTTP_init();
    fona.sendCheckReply(F("AT+SHDISC"),F("OK"),10000U);
    fona.sendCheckReply(F("AT+CNACT?"),F("OK"),10000U);
    fona.sendCheckReply(F("AT+CGMR"),F("OK"),10000U);

    fona.sendCheckReply(F("AT+CMEE=1"),F("OK"),10000U);
    // fona.HTTP_para(F("REDIR"), 1);
    // fona.sendCheckReply(F("AT+CACLOSE=1"),F("OK"),5000U);
    fona.sendCheckReply(F("AT+CACID=1"),F("OK"),5000U);
    fona.sendCheckReply(F("AT+CSSLCFG=\"sslversion\",1,3"),F("OK"),5000U);
    delay(1000);
    fona.sendCheckReply(F("AT+SHSSL=1,\"\""),F("OK"),5000U);
    delay(1000);
    fona.sendCheckReply(F("AT+CSSLCFG=\"ctxindex\",1"),F("OK"),5000U);
    fona.sendCheckReply(F("AT+CSSLCFG=\"ignorertctime\",1,1"),F("OK"),5000U);
    fona.sendCheckReply(F("AT+CSSLCFG=\"protocol\",1,1"),F("OK"),5000U);
    fona.sendCheckReply(F("AT+CSSLCFG=sni,1,\"vpn.leigh.sh\""),F("OK"),5000U);

    fona.sendCheckReply(F("AT+CASSLCFG=1,\"ssl\",1"),F("OK"),5000U);
    fona.sendCheckReply(F("AT+CASSLCFG=1,\"cacert\","),F("OK"),5000U);
    fona.sendCheckReply(F("AT+CASSLCFG=1,\"crindex\",1"),F("OK"),5000U);
    // fona.sendCheckReply(F("AT+CASSLCFG=0,\"protocol\",0"),F("OK"),5000U);

    fona.sendCheckReply(F("AT+SHCONF=\"TIMEOUT\",300"),F("OK"),5000U);
    // fona.sendCheckReply(F("AT+SHCONF=\"URL\",\"https://cms.leigh.sh\""),F("OK"),5000U);
    fona.sendCheckReply(F("AT+SHCONF=\"URL\",\"http://cms.leigh.sh\""),F("OK"),5000U);
    // fona.sendCheckReply(F("AT+SHCONF=\"URL\",\"https://p433xox0.directus.app\""),F("OK"),5000U);
    delay(1000);
    fona.sendCheckReply(F("AT+SHCONF=\"BODYLEN\",63"),F("OK"),5000U);
    delay(1000);
    fona.sendCheckReply(F("AT+SHCONF=\"HEADERLEN\",350"),F("OK"),5000U);
    delay(3000);

    // fona.sendCheckReply(F("AT+CAOPEN=1,\"TCP\",\"httpbin.org\",443"),F("OK"),10000U);

    fona.sendCheckReply(F("AT+SHDISC"),F("OK"),10000U);
    fona.sendCheckReply(F("AT+SHCONN"),F("OK"),10000U);
    // fona.sendCheckReply(F("AT+SHDISC"),F("OK"),10000U);
    // delay(1000);
    fona.sendCheckReply(F("AT+SHCONN"),F("OK"),10000U);
    // delay(1000);
    fona.sendCheckReply(F("AT+SHCONN"),F("OK"),60000U);
    delay(1000);
    // fona.sendCheckReply(F("AT+SHCONN"),F("OK"),10000U);
    // delay(1000);

    fona.sendCheckReply(F("AT+SHCHEAD"),F("OK"),10000U);
    fona.sendCheckReply(F("AT+SHAHEAD=\"Connection\",\"keep-alive\""),F("OK"),10000U);
    // fona.sendCheckReply(F("AT+SHAHEAD=\"Accept\",\"*/*\""),F("OK"),10000U);
    // fona.sendCheckReply(F("AT+SHAHEAD=\"User-Agent\",\"curl/7.47.0\""),F("OK"),10000U);
    // fona.sendCheckReply(F("AT+SHAHEAD=\"Cache-control\",\"no-cache\""),F("OK"),10000U);
    fona.sendCheckReply(F("AT+SHAHEAD=\"Authorization\",\"Bearer nNgr-OJA-K2cNLkfZWQ0B-Xzlrkb9coN\""),F("OK"),10000U);


    // fona.sendCheckReply(F("AT+SHREQ=\"/get?user=jack&password=123\",1"),F("OK"),10000U);
    fona.HTTP_POST(URI,body,bodylen);
    fona.HTTP_POST_end();
    // fona.HTTP_GET("/get?user=jack&password=123");
    // fona.HTTP_GET_end();
    /*
    int counter = 0;
    while (counter < 3 && !fona.postData("POST", URL, body, token, bodylen))
    {
        Serial.println(F("Failed to complete HTTP POST..."));
        counter++;
        delay(1000);
    }
    */
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
    fona.openWirelessConnection(false);
}

// void simOn()
// {
//     // powers on SIM7000

//     // do check for if sim is on
//     pinMode(FONA_PWRKEY, OUTPUT);
//     digitalWrite(FONA_PWRKEY, LOW);
//     delay(1000); // For SIM7000
//     digitalWrite(FONA_PWRKEY, HIGH);
//     delay(4000);
// }

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
    fona.powerDown();
    simOff();
    fonaSerial.flush();
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
    boolean found = tryBegin(9600);

    if (!found)
    {
        // else try at 115200, then set to 9600
        found = tryBegin(115200);

        if (found)
        {
            fona.setBaudrate(9600);
            // Now re-start at 9600
            found = tryBegin(9600);
        }else{
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