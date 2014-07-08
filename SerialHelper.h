#ifndef GB_SerialHelper_h
#define GB_SerialHelper_h

#include <Time.h>

#include "Global.h"
#include "PrintDirty.h"

const char S_WIFI_RESPONSE_WELLCOME[] PROGMEM  = "Welcome to RAK410\r\n";
const char S_WIFI_RESPONSE_ERROR[] PROGMEM  = "ERROR";
const char S_WIFI_RESPONSE_OK[] PROGMEM  = "OK";
const char S_WIFI_GET_[] PROGMEM  = "GET /";
const char S_WIFI_POST_[] PROGMEM  = "POST /"; 

const int WIFI_MAX_SEND_FRAME_SIZE = 1400; // 1400 max from spec

const int WIFI_RESPONSE_DEFAULT_DELAY = 1000; // default delay after "at+" commands 1000ms



/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

class GB_SerialHelper{
private:
  
  static String s_wifiSID;
  static String s_wifiPass;// 8-63 char

  static boolean s_restartWifi;
  static boolean s_restartWifiIfNoResponseAutomatically;

  static int s_sendWifiDataFrameSize;

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

  static void setWifiConfiguration(const String& _s_wifiSID, const String& _s_wifiPass){
    s_wifiSID = _s_wifiSID;
    s_wifiPass = _s_wifiPass;
  }

  static void updateWiFiStatus(){
    if (s_restartWifi){
      checkSerial(false, true);
    }
    //wifiExecuteCommand(F("at+con_status"));
  }

  static void updateSerialMonitorStatus(){
    checkSerial(true, false); // not interruption cause Serial print problems
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
    if (checkWifi || s_restartWifi){
      for (int i = 0; i<2; i++){ // Sometimes first command returns ERROR, two attempts
        showWifiMessage(F("Restarting..."));
        cleanSerialBuffer();
        s_restartWifiIfNoResponseAutomatically = false;
        String input = wifiExecuteRawCommand(F("at+reset=0"), 500); // spec boot time 210   // NOresponse checked wrong
        s_restartWifiIfNoResponseAutomatically = true;

        useSerialWifi = flashStringStartsWith(input, S_WIFI_RESPONSE_WELLCOME);
        if (useSerialWifi) {
          s_restartWifi = false;
          //wifiExecuteCommand(F("at+del_data"));
          //wifiExecuteCommand(F("at+storeenable=0"));
          if(g_isGrowboxStarted){
            loadWifiConfiguration = true;
          }
          break;
        }
        if (useSerialMonitor && input.length() > 0){
          showWifiMessage(F("Not correct wellcome message: "), false);
          GB_PrintDirty::printWithoutCRLF(input);
          Serial.print(FS(S_Next));
          GB_PrintDirty::printHEX(input); 
          Serial.println();
          printDirtyEnd();
        }
      }
    }

    if (useSerialMonitor != oldUseSerialMonitor){
      Serial.print(F("Serial monitor: "));
      if (useSerialMonitor){
        Serial.println(FS(S_enabled));
      } 
      else {
        Serial.println(FS(S_disabled));
      }
      printDirtyEnd();
    }
    if (useSerialWifi != oldUseSerialWifi && (useSerialMonitor || (useSerialMonitor != oldUseSerialMonitor ))){
      if(useSerialWifi){ 
        Serial.print(F("Serial Wi-Fi:"));
        Serial.println(FS(S_connected)); // shows when useSerialMonitor=false
      } 
      else {
        Serial.println(FS(S_disconnected));
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
    showWifiMessage(F("Starting..."));
    boolean isLoaded = startWifiSilent();
    if (isLoaded){
      showWifiMessage(F("Started"));
    } 
    else {
      showWifiMessage(F("Start failed"));
      s_restartWifi = true;
    }
  }

  static GB_COMMAND_TYPE handleSerialEvent(String &input, byte &wifiPortDescriptor, String &postParams){

    input = String();
    input.reserve(100);
    wifiPortDescriptor = 0xFF;
    postParams = String();

    Serial_readString(input, 13); // "at+recv_data="

    if (!flashStringEquals(input, F("at+recv_data="))){
      // Read data from serial manager
      Serial_readString(input); // at first we should read, after manipulate  

      if (flashStringStartsWith(input, S_WIFI_RESPONSE_WELLCOME) || flashStringStartsWith(input, S_WIFI_RESPONSE_ERROR)){
        checkSerial(false, true); // manual restart, or wrong state of Wi-Fi
        return GB_COMMAND_NONE;
      }

      return GB_COMMAND_SERIAL_MONITOR;
    } 
    else {
      // WARNING! We need to do it quick. Standart serial buffer capacity only 64 bytes
      Serial_readString(input, 1); // ends with '\r', cause '\n' will be removed
      byte firstRequestHeaderByte = input[13]; //
      
      if (firstRequestHeaderByte <= 0x07) {        
        // Data Received Successfully
        wifiPortDescriptor = firstRequestHeaderByte; 
    
        Serial_readString(input, 8);  // get full request header

        byte lowByteDataLength = input[20];
        byte highByteDataLength = input[21];
        word dataLength = (((word)highByteDataLength) << 8) + lowByteDataLength;

        // Check HTTP type 
        input = String();
        input.reserve(100);
        dataLength -= Serial_readStringUntil(input, dataLength, S_CRLF);

        boolean isGet = flashStringStartsWith(input, S_WIFI_GET_);
        boolean isPost = flashStringStartsWith(input, S_WIFI_POST_);

        if ((isGet || isPost) && flashStringEndsWith(input, S_CRLF)){
         
          int firstIndex;
          if (isGet){  
            firstIndex = flashStringLength(S_WIFI_GET_) - 1;
          } 
          else {
            firstIndex = flashStringLength(S_WIFI_POST_) - 1;
          }
          int lastIndex = input.indexOf(' ', firstIndex);
          if (lastIndex == -1){
            lastIndex = input.length()-2; // \r\n
          }
          input = input.substring(firstIndex, lastIndex);             

          if (isGet) {
            // We are not interested in this information
            Serial_skipBytes(dataLength); 
            Serial_skipBytes(2); // remove end mark 
            return GB_COMMAND_HTTP_GET;
          } 
          else {
            // Post
            //word dataLength0 = dataLength;
            dataLength -= Serial_skipBytesUntil(dataLength, S_CRLFCRLF); // skip HTTP header
            //word dataLength1 = dataLength;
            dataLength -= Serial_readStringUntil(postParams, dataLength, S_CRLF); // read HTTP data;
            // word dataLength2 = dataLength;           
            Serial_skipBytes(dataLength); // skip remaned endings

            if (flashStringEndsWith(postParams, S_CRLF)){
              postParams = postParams.substring(0, input.length()-2);   
            }
            /*
            postParams += "dataLength0=";
             postParams += dataLength0;
             postParams += ", dataLength1=";
             postParams += dataLength1;
             postParams += ", dataLength2=";
             postParams += dataLength2;
             */
            Serial_skipBytes(2); // remove end mark 
            return GB_COMMAND_HTTP_POST; 
          }
        } 
        else {
          // Unknown HTTP request type
          Serial_skipBytes(dataLength); // remove all data
          Serial_skipBytes(2); // remove end mark 
          return GB_COMMAND_NONE;
        }

      } 
      else if (firstRequestHeaderByte == 0x80) {
        // TCP client connected
        Serial_readString(input, 1); 
        wifiPortDescriptor = input[14]; 
        Serial_skipBytes(8); 
        return GB_COMMAND_HTTP_CONNECTED;

      } 
      else if (firstRequestHeaderByte == 0x81) {
        // TCP client disconnected
        Serial_readString(input, 1); 
        wifiPortDescriptor = input[14]; 
        Serial_skipBytes(8); 
        return GB_COMMAND_HTTP_DISCONNECTED;

      } 
      else if (firstRequestHeaderByte == 0xFF) { 
        // Data received Failed
        Serial_skipBytes(2); // remove end mark and exit quick
        return GB_COMMAND_NONE;

      } 
      else {
        // Unknown packet and it size
        cleanSerialBuffer();
        return GB_COMMAND_NONE;   
      }
    }
    return GB_COMMAND_NONE;  
  } 

  /////////////////////////////////////////////////////////////////////
  //                           HTTP PROTOCOL                         //
  /////////////////////////////////////////////////////////////////////

  static void sendHttpNotFound(const byte wifiPortDescriptor){ 
    sendWifiData(wifiPortDescriptor, F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
    sendWifiCloseConnection(wifiPortDescriptor);
  }
  
  // WARNING! RAK 410 became mad when 2 parallel connections comes. Like with Chrome and POST request, when RAK response 303.
  // Connection for POST request closed by Chrome (not by RAK). And during this time Chrome creates new parallel connection for GET
  // request.
  static void sendHTTPRedirect(const byte &wifiPortDescriptor, const __FlashStringHelper* data){ 
    //const __FlashStringHelper* header = F("HTTP/1.1 303 See Other\r\nLocation: "); // DO not use it with RAK 410
    const __FlashStringHelper* header = F("HTTP/1.1 200 OK (303 doesn't work on RAK 410)\r\nrefresh: 0; url="); 
    sendWifiFrameStart(wifiPortDescriptor, flashStringLength(header) + flashStringLength(data) + flashStringLength(S_CRLFCRLF));
    Serial.print(header);
    Serial.print(data);
    Serial.print(FS(S_CRLFCRLF));
    sendWifiFrameStop();
    sendWifiCloseConnection(wifiPortDescriptor);
  }

  static void sendHttpOK_Header(const byte wifiPortDescriptor){ 
    sendWifiData(wifiPortDescriptor, F("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"));
    sendWifiDataStart(wifiPortDescriptor);
  }

  static boolean sendHttpOK_Data(const byte &wifiPortDescriptor, const __FlashStringHelper* data){
    boolean isSendOK = true;
    if (s_sendWifiDataFrameSize + flashStringLength(data) < WIFI_MAX_SEND_FRAME_SIZE){
      s_sendWifiDataFrameSize += Serial.print(data);
    } 
    else {
      int index = 0;
      while (s_sendWifiDataFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
        char c = flashStringCharAt(data, index++);
        s_sendWifiDataFrameSize += Serial.print(c);
      }
      isSendOK = sendWifiDataStop();
      sendWifiDataStart(wifiPortDescriptor);   
      while (index < flashStringLength(data)){
        char c = flashStringCharAt(data, index++);
        s_sendWifiDataFrameSize += Serial.print(c);
      } 

    }
    return isSendOK;
  }  

  static boolean sendHttpOK_Data(const byte &wifiPortDescriptor, const String &data){
    boolean isSendOK = true;
    if (data.length() == 0){
      return isSendOK;
    }
    if (s_sendWifiDataFrameSize + data.length() < WIFI_MAX_SEND_FRAME_SIZE){
      s_sendWifiDataFrameSize += Serial.print(data);
    } 
    else {
      int index = 0;
      while (s_sendWifiDataFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
        char c = data[index++];
        s_sendWifiDataFrameSize += Serial.print(c);
      }
      isSendOK = sendWifiDataStop();
      sendWifiDataStart(wifiPortDescriptor); 

      while (index < data.length()){
        char c = data[index++];
        s_sendWifiDataFrameSize += Serial.print(c);
      }      
    }
    return isSendOK;
  }

  static void sendHttpOK_PageComplete(const byte &wifiPortDescriptor){  
    sendWifiDataStop();
    sendWifiCloseConnection(wifiPortDescriptor);
  }

private:

  static void showWifiMessage(const __FlashStringHelper* str, boolean newLine = true){ //TODO 
    if (useSerialMonitor){
      Serial.print(FS(S_WIFI));
      Serial.print(str);
      if (newLine){  
        Serial.println();
        printDirtyEnd();        
      }      
    }
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

    if (!wifiExecuteCommand(F("at+scan=0"), 5000)){
      return false;
    } 

    boolean isStationMode = (s_wifiSID.length()>0);    
    if (isStationMode){
      if (s_wifiPass.length() > 0){
        Serial.print(F("at+psk="));
        Serial.print(s_wifiPass);
        if (!wifiExecuteCommand()){
          return false;
        }
      } 

      Serial.print(F("at+connect="));
      Serial.print(s_wifiSID);
      if (!wifiExecuteCommand(0, 5000)){
        return false;
      }

      /*if (!wifiExecuteCommand(F("at+listen=20"))){
       return false;
       }*/

      if (!wifiExecuteCommand(F("at+ipdhcp=0"), 5000)){
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

      if (!wifiExecuteCommand(F("at+ipdhcp=1"), 5000)){
        return false;
      }

      if (!wifiExecuteCommand(F("at+ap=Growbox,1"), 5000)){ // Hidden
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

  static boolean wifiExecuteCommand(const __FlashStringHelper* command = 0, int maxResponseDeleay = WIFI_RESPONSE_DEFAULT_DELAY){   
    String input = wifiExecuteRawCommand(command, maxResponseDeleay);
    if (input.length() == 0){
      // Nothing to do
    } 
    else if (flashStringStartsWith(input, S_WIFI_RESPONSE_OK) && flashStringEndsWith(input, S_CRLF)){
      return true;
    } 
    else if (flashStringStartsWith(input, S_WIFI_RESPONSE_ERROR) && flashStringEndsWith(input, S_CRLF)){
      if (useSerialMonitor){
        byte errorCode = input[5];
        showWifiMessage(F("Error "), false);
        Serial.print(FS(S_0x));
        GB_PrintDirty::printHEX(errorCode, true);
        Serial.println();
        printDirtyEnd();
      }      
    } 
    else {
      if (useSerialMonitor){
        showWifiMessage(FS(S_empty), false);
        GB_PrintDirty::printWithoutCRLF(input);
        Serial.print(FS(S_Next));
        GB_PrintDirty::printHEX(input); 
        Serial.println();
        printDirtyEnd();
      }
    }

    return false;
  }

  static String wifiExecuteRawCommand(const __FlashStringHelper* command, int maxResponseDeleay){

    cleanSerialBuffer();

    if (command == 0){
      Serial.println();
    } 
    else {
      Serial.println(command);
    }

    String input;
    input.reserve(10);
    unsigned long start = millis();
    while(millis() - start <= maxResponseDeleay){
      if (Serial.available()){
        //input += (char) Serial.read(); 
        //input += Serial.readString(); // WARNING! Problems with command at+ipdhcp=0, it returns bytes with minus sign, Error in Serial library
        Serial_readString(input);
      }
    }
    /*
     if (useSerialMonitor){
     showWifiMessage(FS(S_empty), false);
     GB_PrintDirty::printWithoutCRLF(input);
     Serial.print(FS(S_Next));
     GB_PrintDirty::printHEX(input); 
     Serial.println();
     printDirtyEnd();
     }
     */
    if (input.length() == 0){

      if (s_restartWifiIfNoResponseAutomatically){
        s_restartWifi = true;
      }

      if (useSerialMonitor){   
        showWifiMessage(F("No response"), false);
        if (s_restartWifiIfNoResponseAutomatically){
          Serial.print(F(" (reboot)"));
        } 
        Serial.println();
        printDirtyEnd();
      }

    }

    return input;
  }

  /////////////////////////////////////////////////////////////////////
  //                           WIFI PROTOCOL                         //
  /////////////////////////////////////////////////////////////////////

  static void sendWifiFrameStart(const byte portDescriptor, word length){ // 1400 bytes max (Wi-Fi module spec restriction)   
    Serial.print(F("at+send_data="));
    Serial.print(portDescriptor);
    Serial.print(',');
    Serial.print(length);
    Serial.print(',');
  }

  static boolean sendWifiFrameStop(){
    s_restartWifiIfNoResponseAutomatically = false;
    boolean rez = wifiExecuteCommand();
    s_restartWifiIfNoResponseAutomatically = true;
    return rez;
  }

  static void sendWifiData(const byte portDescriptor, const __FlashStringHelper* data){ // INT_MAX (own test) or 1400 bytes max (Wi-Fi spec restriction)
    int length = flashStringLength(data);
    if (length == 0){
      return;
    }
    sendWifiFrameStart(portDescriptor, length);
    Serial.print(data);
    sendWifiFrameStop();
  }

  static void sendWifiDataStart(const byte &wifiPortDescriptor){
    sendWifiFrameStart(wifiPortDescriptor, WIFI_MAX_SEND_FRAME_SIZE);
    s_sendWifiDataFrameSize = 0;
  }

  static boolean sendWifiDataStop(){
    if (s_sendWifiDataFrameSize > 0){
      while (s_sendWifiDataFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
        s_sendWifiDataFrameSize += Serial.write(0x00); // Filler 0x00
      }
    }
    return sendWifiFrameStop();
  } 

  static boolean sendWifiCloseConnection(const byte portDescriptor){
    Serial.print(F("at+cls="));
    Serial.print(portDescriptor);
    return wifiExecuteCommand(); 
  }
  
};

#endif






















