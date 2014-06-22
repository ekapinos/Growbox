#ifndef GB_SerialHelper_h
#define GB_SerialHelper_h

#include <Time.h>

#include "Global.h"
#include "PrintDirty.h"

// TODO optimize it
const String WIFI_RESPONSE_WELLCOME = "Welcome to RAK410\r\n";   // TODO optimize here!
const String WIFI_RESPONSE_ERROR = "ERROR\xFF\r\n";
const String WIFI_RESPONSE_OK = "OK\r\n";
const String WIFI_REQUEST_HEADER = "at+recv_data=";

const word WIFI_HTTP_RESPONSE_OK = 200;
const word WIFI_HTTP_RESPONSE_NOT_FOUND = 404;

const int WIFI_RESPONSE_DELAY_MAX = 5000; // max delay after "at+" commands 5000ms = 5s
const int WIFI_RESPONSE_CHECK_INTERVAL = 100; // during 5s interval, we check for answer every 100 ms

const String WIFI_ACSEESS_POINT_DEFAULT_SID = "Growbox";
const String WIFI_ACSEESS_POINT_DEFAULT_PASS = "ingodwetrust"; // 8-63 chars
const boolean WIFI_ACSEESS_POINT_DEFAULT_HIDDEN = 1;  // 1 - hidden, 0 - not hidden

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

//extern /*volatile*/ boolean g_UseSerialMonitor;
//extern /*volatile*/ boolean g_UseSerialWifi;

class GB_SerialHelper{

public:

  /////////////////////////////////////////////////////////////////////
  //                             STARTUP                             //
  /////////////////////////////////////////////////////////////////////

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
        Serial.println(F("Serial Wi-Fi: connected")); // shows when useSerialMonitor=false
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

  // TODO implemet it in AP mode
  static void updateWiFiStatus(){
    //    String rez = wifiExecuteCommand(F("at+con_status"));
    //    if (WIFI_RESPONSE_OK.equals(rez)){
    //      return;
    //    } 
    //    checkSerial(false, true);
    //    return; 
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

    rez = wifiExecuteCommand(F("at+httpd_open"));
    if (!WIFI_RESPONSE_OK.equals(rez)){
      return false;
    }

    Serial.println("Wi-Fi access point started");
    printDirtyEnd();
    return true;
  }

  /////////////////////////////////////////////////////////////////////
  //                        GROWBOX COMMANDS                         //
  /////////////////////////////////////////////////////////////////////

  static boolean isWifiCommand(const String &input, byte* portDescriptor){   

    (*portDescriptor) = 0xFF;

    if (!input.startsWith(WIFI_REQUEST_HEADER)){ 
      return false;
    }
    byte inputOffset = WIFI_REQUEST_HEADER.length(); // 12 - [0..12]
    byte firstByte = input[inputOffset];
    if (firstByte <= 0x07) {      
       (*portDescriptor) = firstByte;   // Receiving data   
    } 
    else {
      // TCP connect/disconnect - not implemented
    }
    return true;
  }

  static String parseWifiCommandURL(byte portDescriptor, String &input){ // TODO check isWifiCommand()
    String out;
    if (portDescriptor == 0xFF){
      return out;
    }

    byte inputOffset = WIFI_REQUEST_HEADER.length() + 1 + 2 + 4; // Header (13), firstByte(1), Destination port (2), Destination IP (4)
    word dataLength = ((word)((byte)input[inputOffset++]))<<8;
    dataLength += (byte)input[inputOffset++];
    //Serial.print(F("test> ")); Serial.println(dataLength, HEX);
    word lastDataByteOffset = inputOffset + dataLength;
    for (; inputOffset < lastDataByteOffset; inputOffset++){
      out += input[inputOffset];
    }
    return out;
  }

  static boolean sendHttpHeader(byte portDescriptor, word code, word length){ // INT_MAX (own test) or 1400 bytes max (Wi-Fi spec restriction)
    Serial.print(F("at+httpd_send="));
    Serial.print(portDescriptor);
    Serial.print(',');
    Serial.print(code); 
    Serial.print(',');
    Serial.print(length);  
    String rez = wifiExecuteCommand(); 
    return WIFI_RESPONSE_OK.equals(rez);
  }

  static boolean sendHttpDataFrameStart(byte portDescriptor, word length){ // 1024 bytes max (Wi-Fi module restriction)
    Serial.print(F("at+send_data="));
    Serial.print(portDescriptor);
    Serial.print(',');
    Serial.print(length);
    Serial.print(',');
  }

  static boolean sendHttpDataFrameStop(){
    String rez = wifiExecuteCommand(); // TODO move somewere   
    return WIFI_RESPONSE_OK.equals(rez);
  }

 /* static boolean closeHttpConnection(){
    Serial.print(F("at+cls="));
    Serial.print(current_wifiPortDescriptor);
    String rez = wifiExecuteCommand(); 

    current_wifiPortDescriptor = 0xFF; 

    cleanSerialBuffer();
    return WIFI_RESPONSE_OK.equals(rez);
  }*/

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

    for (int i=0; i <= maxResponseDeleay; i += WIFI_RESPONSE_CHECK_INTERVAL){
      delay(WIFI_RESPONSE_CHECK_INTERVAL);
      if (Serial.available()){
        break;
      }
    }
    String input;

    if (!Serial.available()){
      if (useSerialMonitor){
        Serial.print(F("WIFI> internal error: no response"));
      }
      useSerialWifi = false;

      return input;
    }

    while (Serial.available()){
      input += (char) Serial.read();
    }
    if (useSerialMonitor){
      Serial.print(F("WIFI> "));
      GB_PrintDirty::printHEX(input);
      Serial.print(F(" > "));
      Serial.print(input); // \r\n always
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
















