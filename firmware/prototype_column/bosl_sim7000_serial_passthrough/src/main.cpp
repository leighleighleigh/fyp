#include <Arduino.h>
#include <SoftwareSerial.h>

#define SIM_PWRKEY 4
#define SIM_DTR 5 // Connect with solder jumper
#define BOSL_RX 9 // Microcontroller RX
#define BOSL_TX 8 // Microcontroller TX

#define SerialMon Serial
SoftwareSerial SerialAT(BOSL_RX, BOSL_TX); // RX, TX

void simOn()
{
  // powers on SIM7000
  // do check for if sim is on
  pinMode(SIM_PWRKEY, OUTPUT);
  pinMode(BOSL_TX, OUTPUT);
  digitalWrite(BOSL_TX, HIGH);
  pinMode(BOSL_RX, INPUT_PULLUP);
  // digitalWrite(SIM_PWRKEY, LOW);
  // delay(1000); // For SIM7000
  digitalWrite(SIM_PWRKEY, HIGH);
  // delay(4000);
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

void setup()
{
  delay(10);
  // simOff();
  simOn();
  SerialAT.begin(9600);
  SerialMon.begin(9600);
}

void loop()
{
  if (SerialAT.available())
  { // If anything comes in Serial (USB),

    SerialMon.write(SerialAT.read()); // read it and send it out Serial1 (pins 0 & 1)
  }

  if (SerialMon.available())
  { // If anything comes in Serial1 (pins 0 & 1)

    SerialAT.write(SerialMon.read()); // read it and send it out Serial (USB)
  }
}