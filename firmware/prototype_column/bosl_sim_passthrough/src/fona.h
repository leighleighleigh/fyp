#include <Arduino.h>
#include <SoftwareSerial.h>

#define FONA_PWRKEY 4
#define FONA_DTR 5 // Connect with solder jumper
#define FONA_TX 9  // Microcontroller RX
#define FONA_RX 8  // Microcontroller TX

SoftwareSerial fonaSerial = SoftwareSerial(FONA_TX, FONA_RX);

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

boolean tryBegin(uint32_t baud)
{
    // Serial.print("Opening SerialAT at ");
    // Serial.print(baud);
    // Serial.println(" baud.");
    fonaSerial.begin(baud); // Default SIM7000 shield baud rate

    // Serial.println(F("Configuring to 9600 baud"));
    // fonaSerial.println("AT+IPR=9600"); // Set baud rate
    // delay(100);                        // Short pause to let the command run
    // fonaSerial.begin(9600);
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
        // Serial.println(F("Configuring to 9600 baud"));
        fonaSerial.println("AT+IPR=9600"); // Set baud rate

        if (found)
        {
            // Now re-start at 9600
            found = tryBegin(9600);
        }else{
            // Serial.println("COULDNT FIND FONA!");
            return false;
        }
    }
}
