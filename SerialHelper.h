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

const int WIFI_RESPONSE_FRAME_SIZE = 1400; // 1400 max from spec

const int WIFI_RESPONSE_DEFAULT_DELAY = 1000; // default delay after "at+" commands 1000ms



/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

class GB_SerialHelper{
private:
  static const unsigned long Stream_timeout = 1000; // Like in Stram.h

  static String s_wifiSID;
  static String s_wifiPass;// 8-63 char

  static boolean s_restartWifi;
  static boolean s_restartWifiIfNoResponseAutomatically;

  static boolean s_wifiIsHeaderSended;
  static int s_wifiResponseAutoFlushConut;

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
        showWifiStatus(F("Restarting..."));
        cleanSerialBuffer();
        s_restartWifiIfNoResponseAutomatically = false;
        String input = wifiExecuteRawCommand(F("at+reset=0"), 500); // spec boot time 210
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
          showWifiStatus(F("Not correct wellcome message: "), false);
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
    showWifiStatus(F("Starting..."));
    boolean isLoaded = startWifiSilent();
    if (isLoaded){
      showWifiStatus(F("Started"));
    } 
    else {
      showWifiStatus(F("Start failed"));
      s_restartWifi = true;
    }
  }

  static GB_COMMAND_TYPE handleSerialEvent(String &input, byte &wifiPortDescriptor, String &postParams){

    //    int startupFreeMemory = freeMemory();
    //    int maxFreeMemory = startupFreeMemory;

    // Only easy initiated variables should be here
    // boolean isReadError = false;

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
      else if (firstRequestHeaderByte == 0x80 || firstRequestHeaderByte == 0x81) {
        // TCP client connected or disconnected
        /*isWifiRequestClientConnected = (firstRequestHeaderByte == 0x80);   
         isWifiRequestClientDisconnected = (firstRequestHeaderByte == 0x81); */
        Serial_readString(input, 1); 
        wifiPortDescriptor = input[14]; 
        Serial_skipBytes(8); 
        return GB_COMMAND_NONE;

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

  static void startHTTPResponse(const byte &wifiPortDescriptor){  
    s_wifiIsHeaderSended = false;
  }

  static void finishHTTPResponse(const byte &wifiPortDescriptor){  
    if (s_wifiIsHeaderSended){
      stopHttpFrame();
    } 
    else {
      sendHttpNotFoundHeader(wifiPortDescriptor);
    }
    closeConnection(wifiPortDescriptor);
  }


  static boolean sendHttpResponseData(const byte &wifiPortDescriptor, const __FlashStringHelper* data){
    boolean isSendOK = true;
    if (!s_wifiIsHeaderSended){
      sendHttpOKHeader(wifiPortDescriptor); 
      s_wifiIsHeaderSended = true;
      startHttpFrame(wifiPortDescriptor);
    } 
    if (s_wifiResponseAutoFlushConut + flashStringLength(data) < WIFI_RESPONSE_FRAME_SIZE){
      s_wifiResponseAutoFlushConut += Serial.print(data);
    } 
    else {
      int index = 0;
      while (s_wifiResponseAutoFlushConut < WIFI_RESPONSE_FRAME_SIZE){
        char c = flashStringCharAt(data, index++);
        s_wifiResponseAutoFlushConut += Serial.print(c);
      }
      isSendOK = stopHttpFrame();
      startHttpFrame(wifiPortDescriptor);   
      while (index < flashStringLength(data)){
        char c = flashStringCharAt(data, index++);
        s_wifiResponseAutoFlushConut += Serial.print(c);
      } 

    }
    return isSendOK;
  }  

  static boolean sendHttpResponseData(const byte &wifiPortDescriptor, const String &data){
    boolean isSendOK = true;
    if (data.length() == 0){
      return isSendOK;
    }
    if (!s_wifiIsHeaderSended){
      sendHttpOKHeader(wifiPortDescriptor); 
      s_wifiIsHeaderSended = true;
      startHttpFrame(wifiPortDescriptor);
    } 
    if (s_wifiResponseAutoFlushConut + data.length() < WIFI_RESPONSE_FRAME_SIZE){
      s_wifiResponseAutoFlushConut += Serial.print(data);
    } 
    else {
      int index = 0;
      while (s_wifiResponseAutoFlushConut < WIFI_RESPONSE_FRAME_SIZE){
        char c = data[index++];
        s_wifiResponseAutoFlushConut += Serial.print(c);
      }
      isSendOK = stopHttpFrame();
      startHttpFrame(wifiPortDescriptor); 

      while (index < data.length()){
        char c = data[index++];
        s_wifiResponseAutoFlushConut += Serial.print(c);
      }      
    }
    return isSendOK;
  }


private:

  static void showWifiStatus(const __FlashStringHelper* str, boolean newLine = true){ //TODO 
    if (useSerialMonitor){
      Serial.print(FS(S_WIFI));
      Serial.print(str);
      if (newLine){  
        Serial.println();        
      }
      printDirtyEnd();
    }
  }

  /////////////////////////////////////////////////////////////////////
  //                          SERIAL READ                            //
  /////////////////////////////////////////////////////////////////////

  // WARNING! This is adapted copy of Stream.h, Serial.h, and HardwareSerial.h
  // functionality

  /* 
   static int Stream_timedRead1(){
   int c;
   unsigned long _startMillis = millis();
   do {
   c = Serial.read();
   if (c >= 0) return c;
   } while(millis() - _startMillis < 1000);
   return -1;     // -1 indicates timeout
   }
   
   static size_t Stream_readBytes1(char *buffer, size_t length){
   size_t count = 0;
   while (count < length) {
   int c = Stream_timedRead1();
   if (c < 0) break;
   *buffer++ = (char)c;
   count++;
   }
   return count;
   }
   */
  static boolean Serial_timedRead(char* c){
    unsigned long _startMillis = millis();
    unsigned long _currentMillis;
    do {
      if (Serial.available()){
        *c = (char) Serial.read();
        return true;   
      }
      _currentMillis = millis();
    } 
    while(((_currentMillis - _startMillis) < Stream_timeout) || (_currentMillis < _startMillis));  // Overflow check 
    //while((_currentMillis - _startMillis) < Stream_timeout); 
    //while(millis() - _startMillis < Stream_timeout); 
    return false;     // false indicates timeout
  }

  static size_t Serial_readBytes(char *buffer, size_t length) {
    size_t count = 0;
    while (count < length) {
      if (!Serial_timedRead(buffer)){
        break;
      }
      buffer++;
      count++;
    }
    return count;
  }

  static size_t Serial_skipBytes(size_t length) {
    char c;
    size_t count = 0;
    while (count < length) {
      if (!Serial_timedRead(&c)){
        break;
      }
      count++;
    }
    return count;
  }

  static size_t Serial_skipBytesUntil(size_t length, const char PROGMEM* pstr){   
    int pstr_length = flashStringLength(pstr);   
    char matcher[pstr_length];

    char c;
    size_t count = 0;
    while (count < length) {
      if (!Serial_timedRead(&c)){
        break;
      }
      count++;

      for (int i = 1; i < pstr_length; i++){
        matcher[i-1] = matcher[i];  
      }
      matcher[pstr_length-1] = c;
      if (count >= pstr_length && flashStringEquals(matcher, pstr_length, pstr)){
        break;
      } 
    }
    return count;
  }  


  static size_t Serial_readStringUntil(String& str, size_t length, const char PROGMEM* pstr){      
    char c;
    size_t count = 0;
    while (count < length) {
      if (!Serial_timedRead(&c)){
        break;
      }
      count++;
      str +=c;
      if (flashStringEndsWith(str, pstr)){
        break;
      } 
    }
    return count;
  }  

  /*
   
   size_t index = 0;  // maximum target string length is 64k bytes!
   size_t termIndex = 0;
   int c;
   
   while(Serial_timedRead(c)){
   
   if(c != target[index])
   index = 0; // reset index if any char does not match
   
   if( c == target[index]){
   //////Serial.print("found "); Serial.write(c); Serial.print("index now"); Serial.println(index+1);
   if(++index >= targetLen){ // return true if all chars in the target match
   return true;
   }
   }
   
   if(termLen > 0 && c == terminator[termIndex]){
   if(++termIndex >= termLen)
   return false;       // return false if terminate string found before target string
   }
   else
   termIndex = 0;
   }
   return false;
   
   }
   */

  static size_t Serial_readString(String& str, size_t length){
    char buffer[length];
    size_t count = Serial_readBytes(buffer, length);
    str.reserve(str.length() + count);
    for (size_t i = 0; i < count; i++) {
      str += buffer[i];  
    }
    return count;
  }

  static size_t Serial_readString(String& str){

    size_t maxFrameLenght = 100; 
    size_t countInFrame = Serial_readString(str, maxFrameLenght);

    size_t count = countInFrame; 

    while (countInFrame == maxFrameLenght){
      countInFrame = Serial_readString(str, maxFrameLenght); 
      count += countInFrame;
    }
    return count;
  }

  // TODO remove it
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
        showWifiStatus(F("Error "), false);
        Serial.print(FS(S_0x));
        GB_PrintDirty::printHEX(errorCode, true);
        Serial.println();
        printDirtyEnd();
      }      
    } 
    else {
      if (useSerialMonitor){
        showWifiStatus(FS(S_empty), false);
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
     showWifiStatus(FS(S_empty), false);
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
        showWifiStatus(F("No response"), false);
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
  //                           HTTP PROTOCOL                         //
  /////////////////////////////////////////////////////////////////////

  static void sendHttpOKHeader(const byte portDescriptor){ 
    sendWifiData(portDescriptor, F("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"));
  }

  static void sendHttpNotFoundHeader(const byte portDescriptor){ 
    sendWifiData(portDescriptor, F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
  }

  static void startHttpFrame(const byte &wifiPortDescriptor){
    sendWifiFrameStart(wifiPortDescriptor, WIFI_RESPONSE_FRAME_SIZE);
    s_wifiResponseAutoFlushConut = 0;
  }

  static boolean stopHttpFrame(){
    if (s_wifiResponseAutoFlushConut > 0){
      while (s_wifiResponseAutoFlushConut < WIFI_RESPONSE_FRAME_SIZE){
        s_wifiResponseAutoFlushConut += Serial.write(0x00);
      }
    }
    return sendWifiFrameStop();
  } 

  /////////////////////////////////////////////////////////////////////
  //                           WIFI PROTOCOL                         //
  /////////////////////////////////////////////////////////////////////

  static void sendWifiData(const byte portDescriptor, const __FlashStringHelper* data){ // INT_MAX (own test) or 1400 bytes max (Wi-Fi spec restriction)
    int length = flashStringLength(data);
    if (length == 0){
      return;
    }
    sendWifiFrameStart(portDescriptor, length);
    Serial.print(data);
    sendWifiFrameStop();
  }

  static void sendWifiFrameStart(const byte portDescriptor, word length){ // 1024 bytes max (Wi-Fi module restriction)   
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

  static boolean closeConnection(const byte portDescriptor){
    Serial.print(F("at+cls="));
    Serial.print(portDescriptor);
    return wifiExecuteCommand(); 
  }

};

#endif





















