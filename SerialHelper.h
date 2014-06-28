#ifndef GB_SerialHelper_h
#define GB_SerialHelper_h

#include <Time.h>

#include "Global.h"
#include "PrintDirty.h"

// TODO optimize it
const String WIFI_RESPONSE_WELLCOME = "Welcome to RAK410\r\n";   // TODO optimize here!
const String WIFI_RESPONSE_ERROR = "ERROR";//"ERROR\xFF\r\n";
const String WIFI_RESPONSE_OK = "OK";//"OK\r\n";
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
private:
  static String wifiSID;
  static String wifiPass;// 8-63 char

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

  static void setWifiConfiguration(const String& _wifiSID, const String& _wifiPass){
    wifiSID = _wifiSID;
    wifiPass = _wifiPass;
    //    if (g_isGrowboxStarted){
    //      checkSerial(false, true); // restart
    //    }
  }

  static void checkSerial(boolean checkSerialMonitor, boolean checkWifi){

    boolean oldUseSerialMonitor  = useSerialMonitor;
    boolean oldUseSerialWifi     = useSerialWifi;
    boolean serialInUse          = (useSerialMonitor || useSerialWifi);

    if (checkSerialMonitor){
      useSerialMonitor = (digitalRead(USE_SERIAL_MONOTOR_PIN) == SERIAL_ON);
    }

    // Start serial, if we need
    if (!serialInUse && (useSerialMonitor || checkWifi)){
      Serial.begin(115200);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
      } 
      serialInUse = true;
    }

    // If needed Serial already started
    if (!serialInUse){
      return; 
    }

    boolean loadWifiConfiguration = false;
    if (checkWifi){
      for (int i = 0; i<2; i++){ // Sometimes first command returns ERROR, two attempts
        cleanSerialBuffer();
        String input = wifiExecuteCommand(F("at+reset=0"), 500); // spec boot time 210
        useSerialWifi = input.startsWith(WIFI_RESPONSE_WELLCOME);
        if (useSerialWifi) {
          //wifiExecuteCommand(F("at+del_data"));
          //wifiExecuteCommand(F("at+storeenable=0"));
          if(g_isGrowboxStarted){
            loadWifiConfiguration = true;
          }
          break;
        }
        if (useSerialMonitor && input.length() > 0){
          Serial.println(F("WIFI> Not corrent Wi-Fi message"));
          printDirtyEnd();
        }
      }
    }

    if (useSerialMonitor != oldUseSerialMonitor){
      Serial.print(F("Serial monitor: "));
      if (useSerialMonitor){
        Serial.println(F("enabled"));
      } 
      else {
        Serial.println(F("disabled"));
      }
      printDirtyEnd();
    }
    if (useSerialWifi != oldUseSerialWifi && (useSerialMonitor || (useSerialMonitor != oldUseSerialMonitor ))){
      if(useSerialWifi){ 
        Serial.print(F("Serial Wi-Fi: "));
        Serial.println(F("connected")); // shows when useSerialMonitor=false
      } 
      else {
        Serial.println(F("disconnected"));
      }
      printDirtyEnd();
    }

    // Close Serial connection if nessesary
    serialInUse = (useSerialMonitor || useSerialWifi);
    if (!serialInUse){
      Serial.end();
      return;
    }
    if (loadWifiConfiguration){
      startWifi();
    }
  }


  static boolean startWifi(){

    cleanSerialBuffer();

    String rez; 
    String* sid;
    String* pass;

    boolean isStationMode = (wifiSID.length()>0);

    if (isStationMode){
      // Station mode
      rez = wifiExecuteCommand(F("at+scan=0"));
      if (!rez.startsWith(WIFI_RESPONSE_OK)){
        return false;
      }   
      sid = &wifiSID;
      pass = &wifiPass;
    } 
    else {
      // Access point mode
      sid = (String *)&WIFI_ACSEESS_POINT_DEFAULT_SID;
      pass = (String *)&WIFI_ACSEESS_POINT_DEFAULT_PASS;
    }

    if (pass->length() > 0){
      Serial.print(F("at+psk="));
      Serial.print(*pass);
      rez = wifiExecuteCommand();
      if (!rez.startsWith(WIFI_RESPONSE_OK)){
        return false;
      }
    }

    if (isStationMode){
      Serial.print(F("at+connect="));
      Serial.print(*sid);
      rez = wifiExecuteCommand();
      if (!rez.startsWith(WIFI_RESPONSE_OK)){
        return false;
      }

      rez = wifiExecuteCommand(F("at+ipdhcp=0"));
      if (!rez.startsWith(WIFI_RESPONSE_OK)){
        return false;
      }
    }
    else {
      // at+ipstatic=<ip>,<mask>,<gateway>,<dns server1>(0 is valid),<dns server2>(0 is valid)\r\n
      rez = wifiExecuteCommand(F("at+ipstatic=192.168.0.1,255.255.0.0,0.0.0.0,0,0"));
      if (!rez.startsWith(WIFI_RESPONSE_OK)){
        return false;
      }

      rez = wifiExecuteCommand(F("at+ipdhcp=1"));
      if (!rez.startsWith(WIFI_RESPONSE_OK)){
        return false;
      }

      Serial.print(F("at+ap="));
      Serial.print(*sid);
      Serial.print(',');
      Serial.print(WIFI_ACSEESS_POINT_DEFAULT_HIDDEN);
      rez = wifiExecuteCommand();
      if (!rez.startsWith(WIFI_RESPONSE_OK)){
        return false;
      }
    }
    //    rez = wifiExecuteCommand(F("at+httpd_open"));
    //    if (!rez.startsWith(WIFI_RESPONSE_OK)){
    //      return false;
    //    }
    rez = wifiExecuteCommand(F("at+ltcp=80"));
    if (!rez.startsWith(WIFI_RESPONSE_OK)){
      return false;
    }

    if (useSerialMonitor){
      Serial.println(F("Wi-Fi started"));
      printDirtyEnd();
    }
    return true;
  }


  static String readSerial(boolean isWifiInternal = false){

    String input; 

    boolean isReadError = false;

    boolean isWifiRequest = false;
    byte wifiPortDescriptor = 0xFF;

    while (Serial.available()){
      input += (char) readByteFromSerialBuffer(isReadError); // Always use casting to (char) with String object!

      if (!isWifiInternal) {
        if (input.equals(WIFI_REQUEST_HEADER)){ // length compires first 

          byte firstRequestHeaderByte = readByteFromSerialBuffer(isReadError); // first byte

          // Serial.print(F("WIFI-T> ")); Serial.println((byte)firstRequestHeaderByte, HEX); printDirtyEnd();

          if (firstRequestHeaderByte <= 0x07) {   
            // Recive data
            isWifiRequest = true;

            wifiPortDescriptor = firstRequestHeaderByte;
            /*skipByteFromSerialBuffer(isReadError, 6); // skip Destination port and IP
            byte lowByteDataLength = readByteFromSerialBuffer(isReadError);
            byte highByteDataLength = readByteFromSerialBuffer(isReadError);
            word dataLength = (((word)highByteDataLength)<<8) + lowByteDataLength;
            // Serial.print(F("WIFI-T> ")); Serial.println(dataLength); printDirtyEnd();
            */
            // Optimization
            skipByteFromSerialBuffer(isReadError, 8); // skip Destination port, IP and data length

            // Read first line   
            input = "";
            while (Serial.available() && !input.endsWith("\r\n")){
              input += (char) readByteFromSerialBuffer(isReadError); // Always use casting to (char) with String object!
            }
            
            //Serial.print(F("WIFI-T> ")); Serial.println(input); printDirtyEnd(); 
            
            if (input.startsWith("GET /") && input.endsWith("\r\n")){
              int lastIndex = input.indexOf(' ', 4);
              if (lastIndex == -1){
                lastIndex = input.length()-2; // \r\n-1
              }
              input = input.substring(4, lastIndex);
              
              //Serial.print(F("WIFI-T> ")); Serial.println(input); printDirtyEnd(); 
              
              cleanSerialBuffer(); // We are not interested in data which is remained          
              break; // outside cicrle
            } 
            // This is garbage // TODO close port ???
          } 
          else if (firstRequestHeaderByte == 0x80 || firstRequestHeaderByte == 0x81){
            //TCPclient connected, disconnected
            wifiPortDescriptor = readByteFromSerialBuffer(isReadError); // second byte
            // Serial.print(F("WIFI-T> ")); Serial.println(firstRequestHeaderByte, HEX); printDirtyEnd();
          } 
          else {
            // Data recive failed or undocumented command     
          }
          cleanSerialBuffer();
          return String();

        }
        else if (input.startsWith(WIFI_RESPONSE_WELLCOME) || input.startsWith(WIFI_RESPONSE_ERROR)){
          checkSerial(false, true);
          return String();
        }

      }  // if (!isWifiInternal)    
    } // while (Serial.available()) 

    if (useSerialMonitor) {
      if (isWifiInternal){
        Serial.print(F("WIFI> "));
      } 
      else if (isWifiRequest){
        Serial.print(F("WIFI-R> "));
      } 
      else {
        Serial.print(F("SERIAL> "));
      }
      GB_PrintDirty::printWithoutCRLF(input);
      Serial.print(F(" > "));
      GB_PrintDirty::printHEX(input); 
      Serial.println();
      printDirtyEnd();
    } 

    if (isWifiInternal){
      return input; 
    }
    else if (isWifiRequest){
      if (input.startsWith("/")){
        sendHttpHeader(wifiPortDescriptor, WIFI_HTTP_RESPONSE_OK);  
        sendHttpDataFrameStart(wifiPortDescriptor, 7);
        Serial.print(F("Growbox"));
        sendHttpDataFrameStop();
        
      } 
      else {
        sendHttpHeader(wifiPortDescriptor, WIFI_HTTP_RESPONSE_NOT_FOUND);
      } 
      closeConnection(wifiPortDescriptor); // client driven
    } 
    else if (useSerialMonitor){
      input.trim();
      return input;
    } 

    return String();
  } 


  static void updateWiFiStatus(){
    //wifiExecuteCommand(F("at+con_status"));
  }



private:  

  /////////////////////////////////////////////////////////////////////
  //                           SERIAL READ                           //
  /////////////////////////////////////////////////////////////////////

  static byte readByteFromSerialBuffer(boolean &isError){
    if (Serial.available()){
      return Serial.read();
    } 
    else {
      isError = true;
      return 0xFF;
    } 
  }
  /*  
   static boolean appendByteFromSerialBuffer(String &input, byte length = 1){
   for ( int index = 0; (index < length) && Serial.available() ; index++ ){
   input += return Serial.read();
   }
   return (index) == length; 
   }
   */
  static void skipByteFromSerialBuffer(boolean &isError, byte length = 1){
    int index = 0;
    while ((index < length) && Serial.available()){
      Serial.read();
      index++;
    }
    isError = ((index) == length); 
  }

  static void cleanSerialBuffer(){
    delay(250);
    while (Serial.available()){
      Serial.read();
    }
  }

  /////////////////////////////////////////////////////////////////////
  //                           WEB SERVER                            //
  /////////////////////////////////////////////////////////////////////
  /*
  static void readAndSplitWifiRequestHeader(String &input, byte &portDescriptor){ // TODO check isWifiCommand()
   
   portDescriptor = 0xFF;
   
   String out;
   
   byte firstByte = input[13];
   
   if (firstByte == 0x80 || firstByte == 0x81){
   //TCPclient connected, disconnacted
   portDescriptor = input[14]; // second byte
   input = out;
   return;
   } 
   else if (firstByte > 0x07) {  
   // Undocumented
   input = out;
   return; 
   }
   
   // Receiving data      
   portDescriptor = firstByte;   
   
   word dataLength = (((word)input[21])<<8) + (byte)input[20];
   //Serial.print(F("test> ")); Serial.println(dataLength); printDirtyEnd();
   input = input.substring(22, 22 + dataLength);
   input.trim();
   }
   */
  static boolean sendHttpHeader(const byte portDescriptor, word code){ // INT_MAX (own test) or 1400 bytes max (Wi-Fi spec restriction)
    if (code == WIFI_HTTP_RESPONSE_OK){
      Serial.print(F("at+send_data="));
      Serial.print(portDescriptor);
      Serial.print(',');
      Serial.print(63); //length
      Serial.print(',');
      Serial.print(F("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"));  // data
    }
    else {
      Serial.print(F("at+httpd_send="));
      Serial.print(portDescriptor);
      Serial.print(',');
      Serial.print(code); 
      Serial.print(',');
      Serial.print(0);  //length of data
    }
    String rez = wifiExecuteCommand(); 
    return rez.startsWith(WIFI_RESPONSE_OK);
  }

  static boolean sendHttpDataFrameStart(const byte portDescriptor, word length){ // 1024 bytes max (Wi-Fi module restriction)
    Serial.print(F("at+send_data="));
    Serial.print(portDescriptor);
    Serial.print(',');
    Serial.print(length);
    Serial.print(',');
  }

  static boolean sendHttpDataFrameStop(){
    String rez = wifiExecuteCommand(); // TODO move somewere   
    return rez.startsWith(WIFI_RESPONSE_OK);
  }

  static boolean closeConnection(const byte portDescriptor){
    Serial.print(F("at+cls="));
    Serial.print(portDescriptor);
    String rez = wifiExecuteCommand(); 
    return rez.startsWith(WIFI_RESPONSE_OK);
  }


  /////////////////////////////////////////////////////////////////////
  //                        SERIAL Wi-FI CONTROL                     //
  /////////////////////////////////////////////////////////////////////

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

    if (!Serial.available()){
      if (useSerialMonitor){
        Serial.println(F("WIFI> internal error: no response"));
        printDirtyEnd();
      }
      useSerialWifi = false; // TODO check it

      return String();
    }

    return readSerial(true);
  }


};

#endif







































