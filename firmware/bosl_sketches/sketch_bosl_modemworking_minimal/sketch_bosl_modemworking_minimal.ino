/*BoSL board code for green roof SM sensor tests Jan 2022:
 * 
 * Version 0.1 was used to deploy sensors on XX Jan 2022

This code uploads 3 data points to http://www.bosl.com.au/IoT/testing/databases/ with the filename changed under "CLOUD DEFINITIONS" below.

The data are temperature (C)and VWC (%) from a SMT100a soil moisture sensor from Treubner 

The time on the CSV file is 11 hours behind Melbourne time (GMT). The column headers are temperature, EC, and Depth, but should be Temp (C) and VWC (%) which correspond to the data types uploaded.

The program scans the sensors every ~10 minutes, which can be changed using the parameter "ScanInterval". 

For each sensor set, change the Site_ID below to BW_SM1, BW_SM2, etc.

*/



//CLOUD DEFINITIONS
String Site_ID = "LO_BW3"; // This is the ID used for the website URL to store the recorded data
String Website_URL = "www.bosl.com.au/IoT/testing/scripts/WriteMe.php?SiteName="; //Website address e.g. www.bosl.com.au/IoT/E2DesignLabs/scripts/WriteMe.php?SiteName=

//double Max_Upload_Interval = 3600000; // [milliseconds] This is the maximum time between uploads to the web server. e.g. 1 hour is 3,600,000 milliseconds
double Max_Upload_Interval = 10000;

//POWER FUNCTIONS AND VARIABLES
#include <avr/power.h>  //change from avr/power.h in Luke's code due to directory org. on my computer
extern volatile unsigned long timer0_millis;

//GLOBAL VARIABLES FOR TRANSMISSION AND LOGGING INTERVALS
uint32_t lstTransmit = 0; //timestamp of last transmit (milli seconds)
//uint8_t reps = 0;
byte transmit;

//SIM7000 definitions and declarations
#define SIMCOM_7000 // SIM7000A/C/E/G
#define BAUDRATE 9600 // MUST be below 19200 (for stability) but 9600 is more stable
#define CHARBUFF 196 //SIM7000 serial response buffer //longer than 255 will cause issues
#define PWRKEY 4
#define DTR 5 // Connect with solder jumper
#define BOSL_RX 9 // Microcontroller RX
#define BOSL_TX 8 // Microcontroller TX
#include <SoftwareSerial.h>
SoftwareSerial simCom = SoftwareSerial(BOSL_RX, BOSL_TX);
char response[CHARBUFF]; //sim7000 serial response buffer
char Battery_Level[4]; //Battery Level to be read from the SIM7000

//Libraries and sub-procedures for sensors

#include <LowPower.h>

short counter;

//int No;


int a, ScanInterval, MinLogInterval;
String SensorName;
//char response[CHARBUFF];

void setup(){
  /*longer wire need this
  TWBR = 158;
  TWSR |= bit (TWPS1);*/
  
  Serial.begin(BAUDRATE);

  ScanInterval = 10; //set the approximate seconds between each scan: every 30 minutes is 1800 seconds, subtract 20 seconds for program running time

  
 // MinLogInterval = 360; //set the seconds between each logging point

  SensorName = "LO_BW3"; //set to whatever you want


  
     //TURN OFF SIM MODULE
  simOff();

  //BEGIN SIM7000
  Serial.begin(BAUDRATE);
  simCom.begin(BAUDRATE);


  //Serial.println(No);
   
  
  //TURN ON SIM
  simOn();
  
  //INITIALISE SIM7000
  simInit();
  
  //TRANSMIT DATA FOR FIRST TIME
  Battery_Read();
  
  Transmit();

  //TURN OFF SIM
  simOff();
  
  Serial.println(F("Setup Done!"));
}

void loop(){

  Serial.println(F("Sleep"));
  
  Sleepy(ScanInterval);
 
  Serial.println(F(""));
   
    simOn();
    netUnreg();
    Battery_Read();
    Transmit();
    simOff();
 
}


bool sendATcmd(String ATcommand, char* expctAns, unsigned int timeout){
  //sends at command, checks for reply
  uint32_t timeStart;
  bool answer;
  uint8_t a=0;
  
  do{a++;
  
 // Serial.println(ATcommand);
  
  answer=0;
  
  timeStart = 0;

  delay(100);

  while( simCom.available() > 0) {
      simCom.read();    // Clean the input buffer
  }
  
  simCom.println(ATcommand);    // Send the AT command 


  uint8_t i = 0;
  timeStart = millis();
  memset(response, '\0', CHARBUFF);    // Initialize the string

  // this loop waits for the answer

  do{
      if(simCom.available() != 0){    
          response[i] = simCom.read();
          i++;
          // check if the desired answer is in the response of the module
          if (strstr(response, expctAns) != NULL)    
          {
              answer = 1;
          }
      }    
          
          
      
      // Waits for the asnwer with time out
  }
  while((answer == 0) && ((millis() - timeStart) < timeout)); 

  if (expctAns == "0"){
              answer = 1;
          }
  Serial.println(response);
  
  }while(answer == 0 && a < 5);
  
   a = 0;
   return answer;
}



void Battery_Read(){
  if (sendATcmd(F("AT+CBC"), "OK",1000)){
    memset(Battery_Level, '\0', 5); 
    bool end = 0;
    uint8_t x = 0;
    uint8_t j = 0;
    
    //loop through reponce to extract data
    for (uint8_t i=0; i < CHARBUFF; i++){
    
    //string splitting cases
    switch(response[i]){
    case ':':
      x = 9;
      j=0;
      i += 2;
      break;
    
    case ',':
      x++;
      j=0;
      break;
    
    case '\0':
      end = 1;
      break;
    case '\r':
      x++;
      j=0;
      break;
    }
    //write to char arrays
    if (response[i] != ','){
      switch(x){
          case 11:
              Battery_Level[j] = response[i];
          break;             
      }
      //increment char array counter
      j++;
    }
    //break loop when end flag is high
    if (end){
      i = CHARBUFF; 
    }
    }
  }
}


void Transmit(){
  ////TRANSMIT DATA TO WEB

  String dataStr; //Transmit URL


  dataStr = "AT+HTTPPARA=\"URL\",\"";
  dataStr += Website_URL;
  dataStr += Site_ID;
  dataStr += ".csv&T=";
  dataStr += String(321);
  dataStr += "&EC=";
  dataStr += String(123);
  dataStr += "&B=";
  dataStr += String(Battery_Level);
  dataStr += "\"";


  //Register on network
  netReg();
   
  ///***check logic
  //set CSTT - if it is already set, then no need to do again...
  sendATcmd(F("AT+CSTT?"), "OK",1000);   
  if (strstr(response, "mdata.net.au") != NULL){
      //this means the cstt has been set, so no need to set again!
      Serial.println(F("CSTT already set to APN ...no need to set again"));
 } else {
      sendATcmd(F("AT+CSTT=\"mdata.net.au\""), "OK",1000);
  }
    
    
  //close open bearer
  sendATcmd(F("AT+SAPBR=2,1"), "OK",1000);
  if (strstr(response, "1,1") == NULL){
      if (strstr(response, "1,3") == NULL){
      sendATcmd(F("AT+SAPBR=0,1"), "OK",1000);
      }
      sendATcmd(F("AT+SAPBR=1,1"), "OK",1000);
  }
    
  sendATcmd(F("AT+HTTPINIT"), "OK",1000);
  sendATcmd(F("AT+HTTPPARA=\"CID\",1"), "OK",1000);
 
  sendATcmd(dataStr, "OK",1000);
 
  sendATcmd(F("AT+HTTPACTION=0"), "200",2000);
  sendATcmd(F("AT+HTTPTERM"), "OK",1000);

  //close the bearer connection
  sendATcmd(F("AT+SAPBR=0,1"), "OK",1000);
  
  netUnreg();

  lstTransmit = millis();
}


void simOn() {
  //powers on SIM7000
  
  //do check for if sim is on
  pinMode(PWRKEY, OUTPUT);
  pinMode(BOSL_TX, OUTPUT);
  digitalWrite(BOSL_TX, HIGH);
  pinMode(BOSL_RX, INPUT_PULLUP);
  digitalWrite(PWRKEY, LOW);
  xDelay(1000); // For SIM7000
  digitalWrite(PWRKEY, HIGH);
  xDelay(4000);
}

void simOff() {
  //powers off SIM7000
  
  //  TX / RX pins off to save power
  digitalWrite(BOSL_TX, LOW);
  digitalWrite(BOSL_RX, LOW);
  digitalWrite(PWRKEY, LOW);
  xDelay(1200); // For SIM7000
  digitalWrite(PWRKEY, HIGH);
  xDelay(2000);
}


void xDelay(uint32_t tmz){
  //like delay but lower power
  
  uint32_t tmzslc = tmz/64;
  clock_prescale_set(clock_div_64);
  delay(tmzslc);
  clock_prescale_set(clock_div_1);
  cli();
  timer0_millis += 63*tmzslc; 
  sei();
  delay(tmz-64*tmzslc);
}


void netUnreg(){
  //power down cellular functionality
  sendATcmd(F("AT+CFUN=0"), "OK", 1000);
}


void netReg(){
  //register to network
  sendATcmd(F("AT+CFUN=0"), "OK", 1000);
  
  if(sendATcmd(F("AT+CFUN=1"), "+CPIN: READY", 1000) == 0){
      sendATcmd(F("AT+CFUN=6"), "OK", 10000);
      xDelay(10000);
      
      sendATcmd(F("AT+CFUN=1"), "OK", 1000);
  }
  xDelay(2000);
  sendATcmd(F("AT+CREG?"), "+CREG: 0,1", 2000);
}

////initialises sim on arduino startup////
void simInit(){
   
      sendATcmd(F("AT+IPR=9600"),"OK",1000);
      
      sendATcmd(F("ATE0"),"OK",1000);
      
      sendATcmd(F("AT&W0"),"OK",1000);
  
}



////SLEEPS FOR SET TIME////
void Sleepy(uint16_t ScanInterval){ //Sleep Time in seconds
    
    simCom.flush(); // must run before going to sleep
  
    Serial.flush(); // ensures that all messages have sent through serial before arduino sleeps

    
    while(ScanInterval >= 1){
        LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); //8 seconds dosen't work on the 8mhz
        //advance millis timer as it is paused in sleep
        noInterrupts();
        timer0_millis += 1000;
        interrupts();
        
        ScanInterval -= 1;
    }
}
