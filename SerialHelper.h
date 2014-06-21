#ifndef GB_SerialHelper_h
#define GB_SerialHelper_h

#include "Global.h"

// Wi-Fi
const String WIFI_MESSAGE_WELLCOME = "Welcome to RAK410\r\n";
const String WIFI_MESSAGE_ERROR = "ERROR\xFF\r\n";

const int WIFI_RESPONSE_DELAY_MAX = 5000; // max delay after "at+" commands 5000ms = 5s
const int WIFI_RESPONSE_DELAY_INTERVAL = 100; // during 5s interval, we check for answer every 100 ms


const String WIFI_SID = "Hell";
const String WIFI_PASS = "flat65router";

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

//extern /*volatile*/ boolean g_UseSerialMonitor;
//extern /*volatile*/ boolean g_UseSerialWifi;

class GB_SerialHelper{

public:

static /*volatile*/ boolean useSerialMonitor;
static /*volatile*/ boolean useSerialWifi;


  static void printDirtyEnd(){
    if (GB_SerialHelper::useSerialWifi) {
      cleanSerialBuffer();
    }
  }

  static void checkSerial(boolean checkSerialMonitor, boolean checkWifi){

    boolean oldUseSerialMonitor  = useSerialMonitor;
    boolean oldUseSerialWifi     = useSerialWifi;
    boolean serialInUse          = (useSerialMonitor || useSerialWifi);

    if (checkSerialMonitor){
      useSerialMonitor = (digitalRead(USE_SERIAL_MONOTOR_PIN) == SERIAL_ON);
    }

    if (!serialInUse && (useSerialMonitor || checkWifi)){
      Serial.begin(115200);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
      } 
      serialInUse = true;
    }

    if (checkWifi){
      for (int i = 0; i<2; i++){ // Sometimes first command returns ERROR, two attempts
        String input = wifiExecuteCommand(F("at+reset=0"), 500); // spec boot time 210
        useSerialWifi = WIFI_MESSAGE_WELLCOME.equals(input);
        if (useSerialWifi) {
          if(g_isGrowboxStarted){
            startWifi();
          }
          break;
        }
        if (useSerialMonitor && input.length() != 0){
          Serial.print(F("Not corrent Wi-Fi startup message. Expected: "));
          Serial.println(WIFI_MESSAGE_WELLCOME);
          Serial.print(F(" Received: "));
          Serial.println(input);
          printDirtyEnd();
        }
      }
    }

    // If needed Serial already started
    if (!serialInUse){
      return; 
    }

    if (useSerialMonitor != oldUseSerialMonitor){
      if (useSerialMonitor){
        Serial.println(F("Serial monitor: enabled"));
      } 
      else {
        Serial.println(F("Serial monitor: disabled"));
      }
      printDirtyEnd();
    }
    if (useSerialWifi != oldUseSerialWifi){
      if(useSerialWifi){
        Serial.println(F("Serial Wi-Fi: enabled"));
      } 
      else {
        Serial.println(F("Serial Wi-Fi: disabled"));
      }
      printDirtyEnd();
    }

    // Close Serial connection if nessesary
    serialInUse = (useSerialMonitor || useSerialWifi);
    if (!serialInUse){
      Serial.end();
    }
  }

  static void startWifi(){
    String rez = wifiExecuteCommand(F("at+scan=0"));

    Serial.print(rez);
    printDirtyEnd();
  }

private:

  static  String wifiExecuteCommand(const __FlashStringHelper* command, int maxResponseDeleay = -1){
    if (maxResponseDeleay < 0){
      maxResponseDeleay = WIFI_RESPONSE_DELAY_MAX;
    }
    cleanSerialBuffer();
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
      useSerialWifi = false; // TODO show err
      return 0;
    }
    String input="";
    while (Serial.available()){
      input += (char) Serial.read();
    }
    return input;
  }

  static void cleanSerialBuffer(){
    // Clean Serial buffer
    delay(250);
    while (Serial.available()){
      Serial.read();
    }
  }
};

#endif


