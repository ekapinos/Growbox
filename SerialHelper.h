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

const int WIFI_RESPONSE_DELAY_MAX = 5000; // max delay after "at+" commands 5000ms = 5s
const int WIFI_RESPONSE_CHECK_INTERVAL = 100; // during 5s interval, we check for answer every 100 ms

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

//extern /*volatile*/ boolean g_UseSerialMonitor;
//extern /*volatile*/ boolean g_UseSerialWifi;

class GB_SerialHelper{
private:
  static String wifiSID;
  static String wifiPass;// 8-63 char

  static boolean restartWifi;

  static boolean wifiPortDescriptorSendStatus[];

public:

  /////////////////////////////////////////////////////////////////////
  //                             STARTUP                             //
  /////////////////////////////////////////////////////////////////////

  static /*volatile*/ boolean useSerialMonitor;
  static /*volatile*/ boolean useSerialWifi;


  static void printDirtyEnd(){
    if (useSerialWifi) {
      cleanSerialBuffer();
    }
  }

  static void setWifiConfiguration(const String& _wifiSID, const String& _wifiPass){
    wifiSID = _wifiSID;
    wifiPass = _wifiPass;
    //    if (g_isGrowboxStarted){
    //        restartWifi = true;
    //      checkSerial(false, true); // restart
    //    }
  }



  static void updateWiFiStatus(){
    if (restartWifi){
      checkSerial(false, true);
    }
    //wifiExecuteCommand(F("at+con_status"));
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
    if (checkWifi || restartWifi){
      for (int i = 0; i<2; i++){ // Sometimes first command returns ERROR, two attempts
        cleanSerialBuffer();
        String input = wifiExecuteRawCommand(F("at+reset=0"), 500); // spec boot time 210
        useSerialWifi = input.startsWith(WIFI_RESPONSE_WELLCOME);
        if (useSerialWifi) {
          restartWifi = false;
          //wifiExecuteCommand(F("at+del_data"));
          //wifiExecuteCommand(F("at+storeenable=0"));
          if(g_isGrowboxStarted){
            loadWifiConfiguration = true;
          }
          break;
        }
        if (useSerialMonitor && input.length() > 0){
          Serial.print(F("WIFI> Not corrent wellcome message: "));
          GB_PrintDirty::printWithoutCRLF(input);
          Serial.print(F(" > "));
          GB_PrintDirty::printHEX(input); 
          Serial.println();
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
    boolean newSerialInUse = (useSerialMonitor || useSerialWifi);
    if (!newSerialInUse){
      Serial.end();
      return;
    } 
    else if (loadWifiConfiguration){
      startWifi();
    }
  }

  static boolean startWifi(){
    showWiFiStatus(F("Starting wi-fi..."));
    boolean isLoaded = startWifiSilent();
    if (isLoaded){
      showWiFiStatus(F("Wi-Fi started"));
    } 
    else {
      showWiFiStatus(F("Wi-Fi start failed"));
    }
  }

  static boolean handleSerialEvent(String &input, boolean &isWifiRequest, byte &wifiPortDescriptor){

    input = "";  
    isWifiRequest = false; 
    wifiPortDescriptor = 0xFF;

    boolean isReadError = false;

    boolean isWifiRequestClientConnected = false;
    boolean isWifiRequestClientDisconnected = false;

    while (Serial.available()){
      input += (char) readByteFromSerialBuffer(isReadError); // Always use casting to (char) with String object!

      if (input.equals(WIFI_REQUEST_HEADER)){ // length compires first 
        input = "";     
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
          cleanSerialBuffer();
          closeConnection(wifiPortDescriptor);  // This is garbage request, only one attempt allowed
        } 
        else if (firstRequestHeaderByte == 0x80){
          // TCP client connected
          isWifiRequestClientConnected = true;         
          wifiPortDescriptor = readByteFromSerialBuffer(isReadError); // second byte
          cleanSerialBuffer();
          break; // outside cicrle
        }           
        else if (firstRequestHeaderByte == 0x81){
          // TCP client disconnected
          isWifiRequestClientDisconnected = true;
          wifiPortDescriptor = readByteFromSerialBuffer(isReadError); // second byte
          cleanSerialBuffer();
          break; // outside circle
        } 
        else {
          // Data recive failed or undocumented command
          cleanSerialBuffer();     
        }
        return false;
      }
      else if (input.startsWith(WIFI_RESPONSE_WELLCOME) || input.startsWith(WIFI_RESPONSE_ERROR)){
        checkSerial(false, true); // manual restart
        return false;
      }

    } // while (Serial.available()) 

    if (!isWifiRequest){
      input.trim();
    }

    if (useSerialMonitor) {
      if (isWifiRequestClientConnected || isWifiRequestClientDisconnected) {
        Serial.print(F("WIFI> Client ")); 
        Serial.print(wifiPortDescriptor);
        if (isWifiRequestClientConnected){
          Serial.println(F(" connected"));
        } 
        else {
          Serial.println(F(" disconnected"));
        }

      } 
      else {
        if (isWifiRequest){    
          Serial.print(F("WIFI> GET "));
          Serial.println(input);
        } 
        else {
          Serial.print(F("SERIAL> "));
          GB_PrintDirty::printWithoutCRLF(input);
          Serial.print(F(" > "));
          GB_PrintDirty::printHEX(input);
          Serial.println();
        }  
      }
      printDirtyEnd();
    } 

    if (isWifiRequestClientConnected || isWifiRequestClientDisconnected){
      return false; 
    }
    else if (isWifiRequest){
      wifiPortDescriptorSendStatus[wifiPortDescriptor] = false;
      return true;
    } 
    else if (useSerialMonitor){

      return true;
    } 

    return false;
  } 

  static void sendHTTPResponseData(const byte &wifiPortDescriptor, const __FlashStringHelper* data){
    if (!wifiPortDescriptorSendStatus[wifiPortDescriptor]){
      sendHttpOKHeader(wifiPortDescriptor); 
      wifiPortDescriptorSendStatus[wifiPortDescriptor] = true;
    } 
    sendHttpData(wifiPortDescriptor, data);
  }  

  static void sendHTTPResponseData(const byte &wifiPortDescriptor, const String data){
    if (!wifiPortDescriptorSendStatus[wifiPortDescriptor]){
      sendHttpOKHeader(wifiPortDescriptor); 
      wifiPortDescriptorSendStatus[wifiPortDescriptor] = true;
    } 
    sendHttpDataFrameStart(wifiPortDescriptor, data.length());
    Serial.print(data);
    sendHttpDataFrameStop();
  }

  static void sendHTTPResponseDataFrameStart(const byte wifiPortDescriptor, word length){ // 1024 bytes max (Wi-Fi module restriction)
    if (!wifiPortDescriptorSendStatus[wifiPortDescriptor]){
      sendHttpOKHeader(wifiPortDescriptor); 
      wifiPortDescriptorSendStatus[wifiPortDescriptor] = true;
    }    
    sendHttpDataFrameStart(wifiPortDescriptor, length);
  }

  static void sendHTTPResponseDataFrameStop(){
    sendHttpDataFrameStop();
  }


  static void finishHTTPResponse(const byte &wifiPortDescriptor){  
    if (!wifiPortDescriptorSendStatus[wifiPortDescriptor]){
      sendHttpNotFoundHeader(wifiPortDescriptor);
    }
    closeConnection(wifiPortDescriptor);
  }


private:  

  static void showWiFiStatus(const __FlashStringHelper* str){ //TODO 
    if (useSerialMonitor){
      Serial.print(F("WIFI> "));
      Serial.println(str);
      printDirtyEnd();
    }
  }

  /////////////////////////////////////////////////////////////////////
  //                          SERIAL READ                            //
  /////////////////////////////////////////////////////////////////////

  static byte readByteFromSerialBuffer(boolean &isError){
    if (Serial.available()){
      delay(5);
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
    delay(10);
    while (Serial.available()){
      Serial.read();
    }
  }

  /////////////////////////////////////////////////////////////////////
  //                             Wi-FI DEVICE                        //
  /////////////////////////////////////////////////////////////////////

  static boolean startWifiSilent(){

    cleanSerialBuffer();

    if (!wifiExecuteCommand(F("at+scan=0"))){
      return false;
    } 

    boolean isStationMode = (wifiSID.length()>0);    
    if (isStationMode){
      if (wifiPass.length() > 0){
        Serial.print(F("at+psk="));
        Serial.print(wifiPass);
        if (!wifiExecuteCommand()){
          return false;
        }
      } 

      Serial.print(F("at+connect="));
      Serial.print(wifiSID);
      if (!wifiExecuteCommand()){
        return false;
      }

      if (!wifiExecuteCommand(F("at+ipdhcp=0"))){
        return false;
      }
    }
    else {
      if (!wifiExecuteCommand(F("at+psk=ingodwetrust"))){
        return false;
      }  

      // at+ipstatic=<ip>,<mask>,<gateway>,<dns server1>(0 is valid),<dns server2>(0 is valid)\r\n
      if (!wifiExecuteCommand(F("at+ipstatic=192.168.0.1,255.255.0.0,0.0.0.0,0,0"))){
        return false;
      }

      if (!wifiExecuteCommand(F("at+ipdhcp=1"))){
        return false;
      }

      if (!wifiExecuteCommand(F("at+ap=Growbox,1"))){ // Hidden
        return false;
      }
    }

    /*if (!wifiExecuteCommand(F("at+httpd_open"))){
     return false;
     }*/
    if (!wifiExecuteCommand(F("at+ltcp=80"))){
      return false;
    }

    return true;
  }

  static boolean wifiExecuteCommand(const __FlashStringHelper* command = 0, int maxResponseDeleay = -1, boolean rebootOnFalse = true){
    String input = wifiExecuteRawCommand(command,maxResponseDeleay);
    if (input.length() == 0){
      // Nothing to do
    } 
    else if (input.startsWith(WIFI_RESPONSE_OK) && input.endsWith("\r\n")){
      return true;
    } 
    else if (input.startsWith(WIFI_RESPONSE_ERROR) && input.endsWith("\r\n")) {
      if (useSerialMonitor){
        byte errorCode = input[5];
        Serial.print(F("WIFI> error 0x"));
        GB_PrintDirty::printHEX(errorCode);
        Serial.println();
        printDirtyEnd();
      }      
    } 
    else {
      if (useSerialMonitor){
        Serial.print(F("WIFI> "));
        GB_PrintDirty::printWithoutCRLF(input);
        Serial.print(F(" > "));
        GB_PrintDirty::printHEX(input); 
        Serial.println();
        printDirtyEnd();
      }
    }
    if (rebootOnFalse){
      restartWifi = true;
    }

    return false;
  }

  static String wifiExecuteRawCommand(const __FlashStringHelper* command = 0, int maxResponseDeleay = -1){
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

    boolean isReadError = false;
    String input;
    while (Serial.available()){
      input += (char) readByteFromSerialBuffer(isReadError);
    }

    if (input.length() == 0){
      if (useSerialMonitor){   
        Serial.println(F("WIFI> No response"));
        printDirtyEnd();
      }
      restartWifi = true;
    }

    return input;
  }

  /////////////////////////////////////////////////////////////////////
  //                            WEB SERVER                           //
  /////////////////////////////////////////////////////////////////////

  static void sendHttpOKHeader(const byte portDescriptor){ 
    sendHttpData(portDescriptor, F("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"));
  }

  static void sendHttpNotFoundHeader(const byte portDescriptor){ 
    sendHttpData(portDescriptor, F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
    /*
      Serial.print(F("at+httpd_send="));
     Serial.print(portDescriptor);
     Serial.print(',');
     Serial.print(code); 
     Serial.print(F(",0")); //length of data
     return wifiExecuteCommand(); 
     */
  }

  static void sendHttpData(const byte portDescriptor, const __FlashStringHelper* data){ // INT_MAX (own test) or 1400 bytes max (Wi-Fi spec restriction)
    sendHttpDataFrameStart(portDescriptor, flashStringLength(data));
    Serial.print(data);
    sendHttpDataFrameStop();
  }

  static void sendHttpDataFrameStart(const byte portDescriptor, word length){ // 1024 bytes max (Wi-Fi module restriction)   
    Serial.print(F("at+send_data="));
    Serial.print(portDescriptor);
    Serial.print(',');
    Serial.print(length);
    Serial.print(',');
  }

  static void sendHttpDataFrameStop(){
    wifiExecuteCommand();
  }

  static boolean closeConnection(const byte portDescriptor){
    Serial.print(F("at+cls="));
    Serial.print(portDescriptor);
    return wifiExecuteCommand(); 
  }

};

#endif



























































