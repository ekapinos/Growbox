#ifndef GB_SerialHelper_h
#define GB_SerialHelper_h

#include "Global.h"

// Wi-Fi
const String WIFI_RESPONSE_WELLCOME = "Welcome to RAK410\r\n";   // TODO optimize here!
const String WIFI_RESPONSE_ERROR = "ERROR\xFF\r\n";
const String WIFI_RESPONSE_OK = "OK\r\n";

const int WIFI_RESPONSE_DELAY_MAX = 5000; // max delay after "at+" commands 5000ms = 5s
const int WIFI_RESPONSE_CHECK_INTERVAL = 100; // during 5s interval, we check for answer every 100 ms

const String WIFI_ACSEESS_POINT_DEFAULT_SID = "Growbox";
const String WIFI_ACSEESS_POINT_DEFAULT_PASS = "ingodwetrust"; // 8-63 chars
const byte WIFI_ACSEESS_POINT_DEFAULT_HIDDEN = 1;  // 1 - hidden, 0 - not hidden

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

    boolean restartWifi = false;
    if (checkWifi){
      for (int i = 0; i<2; i++){ // Sometimes first command returns ERROR, two attempts
        cleanSerialBuffer();
        String input = wifiExecuteCommand(F("at+reset=0"), 500); // spec boot time 210
        useSerialWifi = WIFI_RESPONSE_WELLCOME.equals(input);
        if (useSerialWifi) {
          if(g_isGrowboxStarted){
            restartWifi = true;
          }
          break;
        }
        if (useSerialMonitor && input.length() != 0){
          Serial.print(F("Not corrent Wi-Fi startup message. Expected: "));
          Serial.println(WIFI_RESPONSE_WELLCOME);
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
        Serial.println(F("Serial Wi-Fi: connected"));
      } 
      else {
        Serial.println(F("Serial Wi-Fi: disconnected"));
      }
      printDirtyEnd();
    }

    // Close Serial connection if nessesary
    serialInUse = (useSerialMonitor || useSerialWifi);
    if (!serialInUse){
      Serial.end();
      return;
    }
    if (restartWifi){
      startWifi();
    }
  }

  static boolean startWifi(){
    cleanSerialBuffer();
    String rez; 

    //    rez = wifiExecuteCommand(F("at+scan=0"));
    //    if (!WIFI_RESPONSE_OK.equals(rez)){
    //      return false;
    //    }

    Serial.print(F("at+psk="));
    Serial.print(WIFI_ACSEESS_POINT_DEFAULT_PASS);
    rez = wifiExecuteCommand();
    if (!WIFI_RESPONSE_OK.equals(rez)){
      return false;
    }

    // at+ipstatic=<ip>,<mask>,<gateway>,<dns server1>(0 is valid),<dns server2>(0 is valid)\r\n
    rez = wifiExecuteCommand(F("at+ipstatic=192.168.0.1,255.255.0.0,0.0.0.0,0,0"));
    if (!WIFI_RESPONSE_OK.equals(rez)){
      return false;
    }

    rez = wifiExecuteCommand(F("at+ipdhcp=1"));
    if (!WIFI_RESPONSE_OK.equals(rez)){
      return false;
    }

    Serial.print(F("at+ap="));
    Serial.print(WIFI_ACSEESS_POINT_DEFAULT_SID);
    Serial.print(',');
    Serial.print(WIFI_ACSEESS_POINT_DEFAULT_HIDDEN);
    rez = wifiExecuteCommand();
    if (!WIFI_RESPONSE_OK.equals(rez)){
      return false;
    }

    Serial.println("Wi-Fi access point started");
    printDirtyEnd();
    return true;
  }

private:

  static String wifiExecuteCommand(const __FlashStringHelper* command = 0, int maxResponseDeleay = -1){
    if (command == 0){
      Serial.println();
    } 
    else {
      Serial.println(command);
    }

    if (maxResponseDeleay < 0){
      maxResponseDeleay = WIFI_RESPONSE_DELAY_MAX;
    }    
    boolean connectionEstablished = false;
    for (int i=0; i <= maxResponseDeleay; i += WIFI_RESPONSE_CHECK_INTERVAL){
      delay(WIFI_RESPONSE_CHECK_INTERVAL);
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
    if (useSerialMonitor){
      Serial.print(F("WF> "));
      Serial.print(input);
      printDirtyEnd();
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




