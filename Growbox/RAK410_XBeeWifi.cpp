#include "RAK410_XBeeWifi.h"

#include "StorageHelper.h"

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

const char S_WIFI_RESPONSE_WELLCOME[] PROGMEM  = "Welcome to RAK410\r\n";
const char S_WIFI_RESPONSE_ERROR[] PROGMEM  = "ERROR";
const char S_WIFI_RESPONSE_OK[] PROGMEM  = "OK";
const char S_WIFI_GET_[] PROGMEM  = "GET /";
const char S_WIFI_POST_[] PROGMEM  = "POST /"; 


RAK410_XBeeWifiClass::RAK410_XBeeWifiClass(): 
c_useSerialWifi(false), c_restartWifi(true), isWifiPrintCommandStarted(false) {
}

boolean RAK410_XBeeWifiClass::isPresent(){ // check if the device is present
  return c_useSerialWifi;
}

/////////////////////////////////////////////////////////////////////
//                             STARTUP                             //
/////////////////////////////////////////////////////////////////////

void RAK410_XBeeWifiClass::updateWiFiStatus(){

  if (c_useSerialWifi && !c_restartWifi){
    return;
  }

  boolean oldUseSerialWifi = c_useSerialWifi;

  if (!c_useSerialWifi){
    Serial1.begin(115200);
    while (!Serial1) {
      ; // wait for serial port to connect. Needed for Leonardo only
    } 
  }

  for (byte i = 0; i < 2; i++){ // Sometimes first command returns ERROR, two attempts

    showWifiMessage(F("Updating status..."));

    String input = wifiExecuteRawCommand(F("at+reset=0"), 500); // spec boot time 210   // NOresponse checked wrong

    c_useSerialWifi = StringUtils::flashStringEquals(input, FS(S_WIFI_RESPONSE_WELLCOME));
    if (c_useSerialWifi) {
      break;
    }
    if (g_useSerialMonitor && input.length() > 0){
      showWifiMessage(F("Not correct wellcome message: "), false);
      PrintUtils::printWithoutCRLF(input);
      Serial.print(FS(S_Next));
      PrintUtils::printHEX(input); 
      Serial.println();
    }
  }

  if (g_useSerialMonitor){
    if (c_useSerialWifi != oldUseSerialWifi ){
      if(c_useSerialWifi){ 
        showWifiMessage(FS(S_Connected));
      } 
      else {
        showWifiMessage(FS(S_Disconnected));
      }
    }
  }

  if (c_useSerialWifi){   
    c_restartWifi = false;
    //wifiExecuteCommand(F("at+del_data"));
    //wifiExecuteCommand(F("at+storeenable=0"));
    if(g_isGrowboxStarted){
      startupWebServer();
    }
  } 
  else {
    Serial.end(); // Close Serial connection if nessesary
  } 

}

boolean RAK410_XBeeWifiClass::startupWebServer(){
  showWifiMessage(F("Starting up Web server..."));
  boolean isLoaded = startupWebServerSilent();
  if (isLoaded){
    showWifiMessage(F("Web server started"));
  } 
  else {
    showWifiMessage(F("Web server start failed"));
    c_restartWifi = true;
  }
  return isLoaded;
}


/////////////////////////////////////////////////////////////////////
//                              WEB SERVER                         //
/////////////////////////////////////////////////////////////////////

RAK410_XBeeWifiClass::RequestType RAK410_XBeeWifiClass::handleSerialEvent(byte &wifiPortDescriptor, String &input, String &postParams){

  wifiPortDescriptor = 0xFF;
  input = postParams = String();

  input.reserve(100);
  Serial_readString(input, 13); // "at+recv_data="

  if (!StringUtils::flashStringEquals(input, F("at+recv_data="))){
    // Read data from serial manager
    Serial_readString(input); // at first we should read, after manipulate  

    if (StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_WELLCOME)) || StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_ERROR))){
      c_restartWifi = true;
      updateWiFiStatus(); // manual restart, or wrong state of Wi-Fi
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
    }

    if (g_useSerialMonitor) {
      showWifiMessage(F("Recive unknown data: "), false);
      PrintUtils::printWithoutCRLF(input);
      Serial.print(FS(S_Next));
      PrintUtils::printHEX(input); 
      Serial.println();
    }

    return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
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
      dataLength -= Serial_readStringUntil(input, dataLength, FS(S_CRLF));

      boolean isGet = StringUtils::flashStringStartsWith(input, FS(S_WIFI_GET_));
      boolean isPost = StringUtils::flashStringStartsWith(input, FS(S_WIFI_POST_));

      if ((isGet || isPost) && StringUtils::flashStringEndsWith(input, FS(S_CRLF))){

        int firstIndex;
        if (isGet){  
          firstIndex = StringUtils::flashStringLength(FS(S_WIFI_GET_)) - 1;
        } 
        else {
          firstIndex = StringUtils::flashStringLength(FS(S_WIFI_POST_)) - 1;
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

          if (g_useSerialMonitor) {
            showWifiMessage(F("Recive from "), false);
            Serial.print(wifiPortDescriptor);
            Serial.print(F(" GET "));
            Serial.println(input);
          }

          return RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET;
        } 
        else {
          // Post
          //word dataLength0 = dataLength;
          dataLength -= Serial_skipBytesUntil(dataLength, FS(S_CRLFCRLF)); // skip HTTP header
          //word dataLength1 = dataLength;
          dataLength -= Serial_readStringUntil(postParams, dataLength, FS(S_CRLF)); // read HTTP data;
          // word dataLength2 = dataLength;           
          Serial_skipBytes(dataLength); // skip remaned endings

          if (StringUtils::flashStringEndsWith(postParams, FS(S_CRLF))){
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

          if (g_useSerialMonitor) {
            showWifiMessage(F("Recive from "), false);
            Serial.print(wifiPortDescriptor);
            Serial.print(F(" POST "));
            Serial.print(input);
            Serial.print(F(", params "));
            Serial.println(postParams);
          }

          return RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST; 
        }
      } 
      else {
        // Unknown HTTP request type
        Serial_skipBytes(dataLength); // remove all data
        Serial_skipBytes(2); // remove end mark 
        if (g_useSerialMonitor) {
          showWifiMessage(F("Recive from "), false);
          Serial.print(wifiPortDescriptor);
          Serial.print(F(" unknown HTTP "));
          PrintUtils::printWithoutCRLF(input);
          Serial.print(FS(S_Next));
          PrintUtils::printHEX(input); 
          Serial.println();
        }

        return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
      }

    } 
    else if (firstRequestHeaderByte == 0x80) {
      // TCP client connected
      Serial_readString(input, 1); 
      wifiPortDescriptor = input[14]; 
      Serial_skipBytes(8); 

      if (g_useSerialMonitor) {
        showWifiMessage(FS(S_Connected), false);
        Serial.print(' ');
        Serial.println(wifiPortDescriptor);
      }

      return RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_CONNECTED;

    } 
    else if (firstRequestHeaderByte == 0x81) {
      // TCP client disconnected
      Serial_readString(input, 1); 
      wifiPortDescriptor = input[14]; 
      Serial_skipBytes(8); 

      if (g_useSerialMonitor) {
        showWifiMessage(FS(S_Disconnected), false);
        Serial.print(' ');
        Serial.println(wifiPortDescriptor);
      }

      return RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_DISCONNECTED;

    } 
    else if (firstRequestHeaderByte == 0xFF) { 
      // Data received Failed
      Serial_skipBytes(2); // remove end mark and exit quick
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;

    } 
    else {
      // Unknown packet and it size
      Serial_skipAll();
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;   
    }
  }
  return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;  
} 

void RAK410_XBeeWifiClass::sendFixedSizeData(const byte portDescriptor, const __FlashStringHelper* data){ // INT_MAX (own test) or 1400 bytes max (Wi-Fi spec restriction)
  int length = StringUtils::flashStringLength(data);
  if (length == 0){
    return;
  }
  sendFixedSizeFrameStart(portDescriptor, length);
  wifiExecuteRawCommandPrint(data);
  sendFixedSizeFrameStop();
}

void RAK410_XBeeWifiClass::sendFixedSizeFrameStart(const byte portDescriptor, word length){ // 1400 bytes max (Wi-Fi module spec restriction)   
  wifiExecuteRawCommandPrint(F("at+send_data="));
  wifiExecuteRawCommandPrint(portDescriptor);
  wifiExecuteRawCommandPrint(',');
  wifiExecuteRawCommandPrint(length);
  wifiExecuteRawCommandPrint(',');
}

void RAK410_XBeeWifiClass::sendFixedSizeFrameData(const __FlashStringHelper* data){
  wifiExecuteRawCommandPrint(data);
}

boolean RAK410_XBeeWifiClass::sendFixedSizeFrameStop(){

  String input = wifiExecuteRawCommand();
  if (StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_OK)) && StringUtils::flashStringEndsWith(input, FS(S_CRLF))){
    return true;
  } 
  else {
    return false;  // maybe client disconnected
  }
}

void RAK410_XBeeWifiClass::sendAutoSizeFrameStart(const byte &wifiPortDescriptor){
  sendFixedSizeFrameStart(wifiPortDescriptor, WIFI_MAX_SEND_FRAME_SIZE);
  c_autoSizeFrameSize = 0;
}

boolean RAK410_XBeeWifiClass::sendAutoSizeFrameData(const byte &wifiPortDescriptor, const __FlashStringHelper* data){
  boolean isSendOK = true;
  if (c_autoSizeFrameSize + StringUtils::flashStringLength(data) < WIFI_MAX_SEND_FRAME_SIZE){
    c_autoSizeFrameSize += wifiExecuteRawCommandPrint(data);
  } 
  else {
    size_t index = 0;
    while (c_autoSizeFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
      char c = StringUtils::flashStringCharAt(data, index++);
      c_autoSizeFrameSize += wifiExecuteRawCommandPrint(c);
    }
    isSendOK = sendAutoSizeFrameStop();
    sendAutoSizeFrameStart(wifiPortDescriptor);   
    while (index < StringUtils::flashStringLength(data)){
      char c = StringUtils::flashStringCharAt(data, index++);
      c_autoSizeFrameSize += wifiExecuteRawCommandPrint(c);
    } 

  }
  return isSendOK;
}  

boolean RAK410_XBeeWifiClass::sendAutoSizeFrameData(const byte &wifiPortDescriptor, const String &data){
  boolean isSendOK = true;
  if (data.length() == 0){
    return isSendOK;
  }
  if (c_autoSizeFrameSize + data.length() < WIFI_MAX_SEND_FRAME_SIZE){
    c_autoSizeFrameSize += wifiExecuteRawCommandPrint(data);
  } 
  else {
    size_t index = 0;
    while (c_autoSizeFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
      char c = data[index++];
      c_autoSizeFrameSize += wifiExecuteRawCommandPrint(c);
    }
    isSendOK = sendAutoSizeFrameStop();
    sendAutoSizeFrameStart(wifiPortDescriptor); 

    while (index < data.length()){
      char c = data[index++];
      c_autoSizeFrameSize += wifiExecuteRawCommandPrint(c);
    }      
  }
  return isSendOK;
}

boolean RAK410_XBeeWifiClass::sendAutoSizeFrameStop(){
  if (c_autoSizeFrameSize > 0){
    while (c_autoSizeFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
      c_autoSizeFrameSize += Serial1.write(0x00); // Filler 0x00
    }
  }
  return sendFixedSizeFrameStop();
} 

boolean RAK410_XBeeWifiClass::sendCloseConnection(const byte wifiPortDescriptor){
  wifiExecuteRawCommandPrint(F("at+cls="));
  wifiExecuteRawCommandPrint(wifiPortDescriptor);
  return wifiExecuteCommand(); 
}

//private:


boolean RAK410_XBeeWifiClass::startupWebServerSilent(){

  if (!wifiExecuteCommand(F("at+pwrmode=0"))){
    return false;
  }

  if (!wifiExecuteCommand(F("at+scan=0"), 5000)){
    return false;
  } 

  String wifiSSID = GB_StorageHelper.getWifiSSID();
  String wifiPass = GB_StorageHelper.getWifiPASS();

  if (wifiPass.length() > 0){
    wifiExecuteRawCommandPrint(F("at+psk="));
    wifiExecuteRawCommandPrint(wifiPass);
    if (!wifiExecuteCommand()){
      return false;
    }
  }

  if (GB_StorageHelper.isWifiStationMode()){
    wifiExecuteRawCommandPrint(F("at+connect="));
    wifiExecuteRawCommandPrint(wifiSSID);
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

    // at+ipstatic=<ip>,<mask>,<gateway>,<dns server1>(0 is valid),<dns server2>(0 is valid)\r\n
    if (!wifiExecuteCommand(F("at+ipstatic=192.168.0.1,255.255.0.0,0.0.0.0,0,0"))){
      return false;
    }

    if (!wifiExecuteCommand(F("at+ipdhcp=1"), 5000)){
      return false;
    }

    wifiExecuteRawCommandPrint(F("at+ap="));
    wifiExecuteRawCommandPrint(wifiSSID);
    wifiExecuteRawCommandPrint(F(",1"));
    if (!wifiExecuteCommand(0, 5000)){ 
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

boolean RAK410_XBeeWifiClass::wifiExecuteCommand(const __FlashStringHelper* command, size_t maxResponseDeleay){   
  String input = wifiExecuteRawCommand(command, maxResponseDeleay);
  if (StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_OK)) && StringUtils::flashStringEndsWith(input, FS(S_CRLF))){
    return true;
  } 
  else if (input.length() == 0){
    c_restartWifi = true;

    if (g_useSerialMonitor){   
      showWifiMessage(F("No response. Wi-fi will reboot"));
    }
    // Nothing to do
  } 
  else if (StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_ERROR)) && StringUtils::flashStringEndsWith(input, FS(S_CRLF))){
    if (g_useSerialMonitor){
      byte errorCode = input[5];
      showWifiMessage(F("Error "), false);
      Serial.print(StringUtils::byteToHexString(errorCode, true));
      Serial.println();
    }      
  } 
  else {
    if (g_useSerialMonitor){
      showWifiMessage(0, false);
      PrintUtils::printWithoutCRLF(input);
      Serial.print(FS(S_Next));
      PrintUtils::printHEX(input); 
      Serial.println();
    }
  }

  return false;
}

String RAK410_XBeeWifiClass::wifiExecuteRawCommand(const __FlashStringHelper* command, size_t maxResponseDeleay){

  Serial_skipAll();

  if (command == 0){   
    Serial1.println();
  } 
  else {
    Serial1.println(command);
  }

  if (g_useSerialMonitor){
    if (isWifiPrintCommandStarted){
      if (command == 0){   
        Serial.println();
      } 
      else {
        Serial.println(command);
      }    
    } 
    else {
      if (command == 0){   
        showWifiMessage(F("Empty command"));
      } 
      else {
        showWifiMessage(command);
      }  
    }
  }
  isWifiPrintCommandStarted = false;

  String input;
  input.reserve(10);
  unsigned long start = millis();
  while(millis() - start <= maxResponseDeleay){
    if (Serial1.available()){
      //input += (char) Serial.read(); 
      //input += Serial.readString(); // WARNING! Problems with command at+ipdhcp=0, it returns bytes with minus sign, Error in Serial library
      Serial_readString(input);

      //showWifiMessage(FS(S_empty), false);
      //Serial.println(input);

    }
  }
  return input;
}

RAK410_XBeeWifiClass RAK410_XBeeWifi;













