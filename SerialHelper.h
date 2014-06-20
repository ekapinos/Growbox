#ifndef GB_SerialHelper_h
#define GB_SerialHelper_h

// Wi-Fi
const String WIFI_MESSAGE_WELLCOME = "Welcome to RAK410\r\n";
const String WIFI_MESSAGE_ERROR = "ERROR\xFF\r\n";

const int WIFI_RESPONSE_DELAY_MAX = 5000; // max delay after "at+" commands 5000ms = 5s
const int WIFI_RESPONSE_DELAY_INTERVAL = 100; // during 5s interval, we check for answer every 100 ms


String WIFI_SID = "Hell";
String WIFI_PASS = "flat65router";

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

extern /*volatile*/ boolean g_UseSerialMonitor;
extern /*volatile*/ boolean g_UseSerialWifi;

class GB_SerialHelper{
  
public:

  static void printEnd(){
    if (g_UseSerialWifi) {
      delay(250);
      while (Serial.available()){
        Serial.read();
      }
    }
  }
  
  static void checkSerial(boolean checkSerialMonitor, boolean checkWifi){

    boolean oldUseSerialMonitor  = g_UseSerialMonitor;
    boolean oldUseSerialWifi     = g_UseSerialWifi;
    boolean serialInUse          = (g_UseSerialMonitor || g_UseSerialWifi);

    if (checkSerialMonitor){
      g_UseSerialMonitor = (digitalRead(USE_SERIAL_MONOTOR_PIN) == SERIAL_ON);

    }

    if (!serialInUse && (g_UseSerialMonitor || checkWifi)){
      Serial.begin(115200);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
      } 
      serialInUse = true;
    }

    if (checkWifi){

      for (int i = 0; i<2; i++){ // Sometimes first command returns ERROR, two attempts

        delay(250);

        // Clean Serial buffer
        while (Serial.available()){
          Serial.read();
        }

        String input = wifiExecuteCommand(F("at+reset=0")); // spec boot time

        g_UseSerialWifi = WIFI_MESSAGE_WELLCOME.equals(input);
        if (g_UseSerialWifi) {
          if(BOOT_RECORD.isCorrect()){
            startWifi();
          }
          break;
        }
        if (g_UseSerialMonitor && input.length() != 0){
          Serial.print(F("Not corrent Wi-Fi startup message. Expected: "));
          Serial.println(WIFI_MESSAGE_WELLCOME);
          Serial.print(F(" Received: "));
          Serial.println(input);
          printEnd();
        }
      }
    }

    // If needed Serial already started
    if (!serialInUse){
      return; 
    }

    if (g_UseSerialMonitor != oldUseSerialMonitor){
      if (g_UseSerialMonitor){
        Serial.println(F("Serial monitor: enabled"));
      } 
      else {
        Serial.println(F("Serial monitor: disabled"));
      }
      printEnd();
    }
    if (g_UseSerialWifi != oldUseSerialWifi){
      if(g_UseSerialWifi){
        Serial.println(F("Serial Wi-Fi: enabled"));
      } 
      else {
        Serial.println(F("Serial Wi-Fi: disabled"));
      }
      printEnd();
    }

    // Close Serial connection if nessesary
    serialInUse = (g_UseSerialMonitor || g_UseSerialWifi);
    if (!serialInUse){
      Serial.end();
    }
  }

  static void startWifi(){
    String rez = wifiExecuteCommand(F("at+scan=0"));

    Serial.print(rez);
    printEnd();
  }

private:

  static  String wifiExecuteCommand(const __FlashStringHelper* command, int maxResponseDeleay = -1){

    if (maxResponseDeleay < 0){
      maxResponseDeleay = WIFI_RESPONSE_DELAY_MAX;
    }

    // Clean Serial buffer
    while (Serial.available()){
      Serial.read();
    }

    Serial.println(command);

    boolean connectionEstablished = false;
    for (int i=0; i <= maxResponseDeleay; i += WIFI_RESPONSE_DELAY_INTERVAL){
      delay(WIFI_RESPONSE_DELAY_INTERVAL);
      if (Serial.available()){
        connectionEstablished = true;
        break;
      }
    }
    if (!connectionEstablished){
      g_UseSerialWifi = false; // TODO show err
      return 0;
    }
    String input="";
    while (Serial.available()){
      input += (char) Serial.read();
    }
    return input;
  }
};

#endif

