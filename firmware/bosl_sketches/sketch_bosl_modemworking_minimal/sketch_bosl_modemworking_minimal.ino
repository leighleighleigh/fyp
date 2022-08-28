/*BoSL board code modified for Leigh Oliver FYP
This sketch intended to provide a minimal working example of SIM7000G HTTP requests.
'Low-power' method have been removed from the original version.
Uploads placeholder data to http://www.bosl.com.au/IoT/testing/databases/ with the filename changed under "CLOUD DEFINITIONS" below.
*/
#include <avr/power.h>  
#include <SoftwareSerial.h>

#define SIMCOM_7000 // SIM7000A/C/E/G
#define BAUDRATE 9600 // MUST be below 19200 (for stability) but 9600 is more stable
#define CHARBUFF 196 //SIM7000 serial response buffer //longer than 255 will cause issues
#define PWRKEY 4
#define DTR 5 // Connect with solder jumper
#define BOSL_RX 9 // Microcontroller RX
#define BOSL_TX 8 // Microcontroller TX

//CLOUD DEFINITIONS
String Site_ID = "LO_BW3"; // This is the ID used for the website URL to store the recorded data
String Website_URL = "www.bosl.com.au/IoT/testing/scripts/WriteMe.php?SiteName="; 

//POWER FUNCTIONS AND VARIABLES
unsigned long transmitInterval = 60000; // Milliseconds

//GLOBAL VARIABLES FOR TRANSMISSION AND LOGGING INTERVALS
SoftwareSerial simCom = SoftwareSerial(BOSL_RX, BOSL_TX);
char response[CHARBUFF]; //sim7000 serial response buffer
char simBattLevel[4]; //Battery Level to be read from the SIM7000

void setup(){
  Serial.begin(BAUDRATE);
 
  //TURN OFF SIM MODULE
  simOff();

  //BEGIN SIM7000
  Serial.begin(BAUDRATE);
  simCom.begin(BAUDRATE);

  //TURN ON SIM
  simOn();
  
  //INITIALISE SIM7000
  simInit();
  
  //TRANSMIT DATA FOR FIRST TIME
  readBatteryLevel();
  
  transmitToWeb();

  //TURN OFF SIM
  simOff();
  
  Serial.println(F("Setup Done!"));
}

void loop(){
  Serial.println("Sleeping for " + String(transmitInterval/1000));
  delay(transmitInterval);
 
  Serial.println(F(""));
   
    simOn();
    netUnreg();
    readBatteryLevel();
    transmitToWeb();
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



void readBatteryLevel(){
  if (sendATcmd(F("AT+CBC"), "OK",1000)){
    memset(simBattLevel, '\0', 5); 
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
              simBattLevel[j] = response[i];
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


void transmitToWeb(){
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
  dataStr += String(simBattLevel);
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
}


void simOn() {
  //powers on SIM7000
  
  //do check for if sim is on
  pinMode(PWRKEY, OUTPUT);
  pinMode(BOSL_TX, OUTPUT);
  digitalWrite(BOSL_TX, HIGH);
  pinMode(BOSL_RX, INPUT_PULLUP);
  digitalWrite(PWRKEY, LOW);
  delay(1000); // For SIM7000
  digitalWrite(PWRKEY, HIGH);
  delay(4000);
}

void simOff() {
  //powers off SIM7000
  
  //  TX / RX pins off to save power
  digitalWrite(BOSL_TX, LOW);
  digitalWrite(BOSL_RX, LOW);
  digitalWrite(PWRKEY, LOW);
  delay(1200); // For SIM7000
  digitalWrite(PWRKEY, HIGH);
  delay(2000);
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
      delay(10000);
      
      sendATcmd(F("AT+CFUN=1"), "OK", 1000);
  }
  delay(2000);
  sendATcmd(F("AT+CREG?"), "+CREG: 0,1", 2000);
}

////initialises sim on arduino startup////
void simInit(){
   
      sendATcmd(F("AT+IPR=9600"),"OK",1000);
      
      sendATcmd(F("ATE0"),"OK",1000);
      
      sendATcmd(F("AT&W0"),"OK",1000);
  
}
