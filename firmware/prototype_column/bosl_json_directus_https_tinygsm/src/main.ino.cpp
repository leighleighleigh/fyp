# 1 "/tmp/tmpq_abrw24"
#include <Arduino.h>
# 1 "/home/leigh/Documents/GitHub/fyp/firmware/prototype_column/bosl_json_directus_https_tinygsm/src/main.ino"
# 24 "/home/leigh/Documents/GitHub/fyp/firmware/prototype_column/bosl_json_directus_https_tinygsm/src/main.ino"
#define PWRKEY 4
#define DTR 5
#define BOSL_RX 9
#define BOSL_TX 8
#define __AVR_ATmega328P__ 





#define TINY_GSM_MODEM_SIM7000SSL 
# 43 "/home/leigh/Documents/GitHub/fyp/firmware/prototype_column/bosl_json_directus_https_tinygsm/src/main.ino"
#define SerialMon Serial



#ifndef __AVR_ATmega328P__
#define SerialAT Serial1


#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(BOSL_RX, BOSL_TX);
#endif





#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 200
#endif


#define DUMP_AT_COMMANDS 


#define TINY_GSM_DEBUG SerialMon





#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200



#define TINY_GSM_YIELD() { delay(2); }



#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false


#define GSM_PIN ""





const char apn[] = "mdata.net.au";
const char gprsUser[] = "";
const char gprsPass[] = "";


const char wifiSSID[] = "YourSSID";
const char wifiPass[] = "YourWiFiPass";


const char server[] = "vsh.pp.ua";
const char resource[] = "/TinyGSM/logo.txt";
const int port = 443;

#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>


#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClientSecure client(modem);
HttpClient http(client, server, port);
void simOn();
void simOff();
void setup();
void loop();
#line 134 "/home/leigh/Documents/GitHub/fyp/firmware/prototype_column/bosl_json_directus_https_tinygsm/src/main.ino"
void simOn() {



  pinMode(PWRKEY, OUTPUT);
  pinMode(BOSL_TX, OUTPUT);
  digitalWrite(BOSL_TX, HIGH);
  pinMode(BOSL_RX, INPUT_PULLUP);
  digitalWrite(PWRKEY, LOW);
  delay(1000);
  digitalWrite(PWRKEY, HIGH);
  delay(4000);
}

void simOff() {



  digitalWrite(BOSL_TX, LOW);
  digitalWrite(BOSL_RX, LOW);
  digitalWrite(PWRKEY, LOW);
  delay(1200);
  digitalWrite(PWRKEY, HIGH);
  delay(2000);
}


void setup() {

  SerialMon.begin(115200);
  delay(10);




  simOff();
  simOn();

  SerialMon.println("Wait...");



  SerialAT.begin(9600);
  delay(6000);



  SerialMon.println("Initializing modem...");
  modem.restart();


  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

#if TINY_GSM_USE_GPRS

  if (GSM_PIN && modem.getSimStatus() != 3) { modem.simUnlock(GSM_PIN); }
#endif
}

void loop() {
#if TINY_GSM_USE_WIFI

  SerialMon.print(F("Setting SSID/password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE

  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isNetworkConnected()) { SerialMon.println("Network connected"); }

#if TINY_GSM_USE_GPRS

  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");

  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }
#endif

  SerialMon.print(F("Synchronising to NTP timeserver...\n"));
  modem.NTPServerSync();

  SerialMon.print(F("Performing HTTPS GET request... "));
  http.connectionKeepAlive();
  http.sendHeader(F("Accept: text/plain"));
  http.sendHeader(F("Cache-Control: no-cache"));
  http.sendHeader(F("Transfer-Encoding: chunked"));

  int err = http.get(resource);
  if (err != 0) {
    SerialMon.println(F("failed to connect"));
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  SerialMon.print(F("Response status code: "));
  SerialMon.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  SerialMon.println(F("Response Headers:"));
  while (http.headerAvailable()) {
    String headerName = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    SerialMon.println("    " + headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    SerialMon.print(F("Content length is: "));
    SerialMon.println(length);
  }
  if (http.isResponseChunked()) {
    SerialMon.println(F("The response is chunked"));
  }

  String body = http.responseBody();
  SerialMon.println(F("Response:"));
  SerialMon.println(body);

  SerialMon.print(F("Body length is: "));
  SerialMon.println(body.length());



  http.stop();
  SerialMon.println(F("Server disconnected"));

#if TINY_GSM_USE_WIFI
  modem.networkDisconnect();
  SerialMon.println(F("WiFi disconnected"));
#endif
#if TINY_GSM_USE_GPRS
  modem.gprsDisconnect();
  SerialMon.println(F("GPRS disconnected"));
#endif


  while (true) { delay(1000); }
}