#include "RAK410_XBeeWifi.h"


/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

const char S_WIFI_RESPONSE_WELLCOME[] PROGMEM  = "Welcome to RAK410\r\n";
const char S_WIFI_RESPONSE_ERROR[] PROGMEM  = "ERROR";
const char S_WIFI_RESPONSE_OK[] PROGMEM  = "OK";
const char S_WIFI_GET_[] PROGMEM  = "GET /";
const char S_WIFI_POST_[] PROGMEM  = "POST /"; 


RAK410_XBeeWifiClass::RAK410_XBeeWifiClass(): 
useSerialWifi (false), s_restartWifi(false), s_restartWifiIfNoResponseAutomatically(true) {
}
/////////////////////////////////////////////////////////////////////
//                             STARTUP                             //
/////////////////////////////////////////////////////////////////////


void RAK410_XBeeWifiClass::setWifiConfiguration(const String& _s_wifiSID, const String& _s_wifiPass){
  s_wifiSID = _s_wifiSID;
  s_wifiPass = _s_wifiPass;
}

void RAK410_XBeeWifiClass::updateWiFiStatus(){
  if (s_restartWifi){
    checkSerial();
  }
}

void RAK410_XBeeWifiClass::checkSerial(){

  boolean oldUseSerialWifi     = useSerialWifi;

  if (!useSerialWifi){
    Serial1.begin(115200);
    while (!Serial1) {
      ; // wait for serial port to connect. Needed for Leonardo only
    } 
  }

  boolean loadWifiConfiguration = false;

  for (int i = 0; i<2; i++){ // Sometimes first command returns ERROR, two attempts
    showWifiMessage(F("Restarting..."));

    String input = wifiExecuteRawCommand(F("at+reset=0"), 500); // spec boot time 210   // NOresponse checked wrong

    useSerialWifi = StringUtils::flashStringEquals(input, S_WIFI_RESPONSE_WELLCOME);
    if (useSerialWifi) {
      s_restartWifi = false;
      //wifiExecuteCommand(F("at+del_data"));
      //wifiExecuteCommand(F("at+storeenable=0"));
      if(g_isGrowboxStarted){
        loadWifiConfiguration = true;
      }
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
    if (useSerialWifi != oldUseSerialWifi ){
      if(useSerialWifi){ 
        Serial.print(F("Serial Wi-Fi:"));
        Serial.println(FS(S_connected)); // shows when useSerialMonitor=false
      } 
      else {
        Serial.println(FS(S_disconnected));
      }
    }
  }

  // Close Serial connection if nessesary // TODO
  if (!useSerialWifi){
    Serial.end();
    return;
  } 
  else if (loadWifiConfiguration){
    startWifi();
  }
}

boolean RAK410_XBeeWifiClass::startWifi(){
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

void RAK410_XBeeWifiClass::cleanSerialBuffer(){
  delay(10);
  while (Serial1.available()){
    Serial1.read();
  }
}



//public:
/////////////////////////////////////////////////////////////////////
//                           WIFI PROTOCOL                         //
/////////////////////////////////////////////////////////////////////

RAK410_XBeeWifiClass::RequestType RAK410_XBeeWifiClass::handleSerialEvent(byte &wifiPortDescriptor, String &input, String &postParams){

  wifiPortDescriptor = 0xFF;
  input = postParams = String();

  input.reserve(100);
  Serial_readString(input, 13); // "at+recv_data="

  if (!StringUtils::flashStringEquals(input, F("at+recv_data="))){
    // Read data from serial manager
    Serial_readString(input); // at first we should read, after manipulate  

    if (StringUtils::flashStringStartsWith(input, S_WIFI_RESPONSE_WELLCOME) || StringUtils::flashStringStartsWith(input, S_WIFI_RESPONSE_ERROR)){
      checkSerial(); // manual restart, or wrong state of Wi-Fi
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
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
      dataLength -= Serial_readStringUntil(input, dataLength, S_CRLF);

      boolean isGet = StringUtils::flashStringStartsWith(input, S_WIFI_GET_);
      boolean isPost = StringUtils::flashStringStartsWith(input, S_WIFI_POST_);

      if ((isGet || isPost) && StringUtils::flashStringEndsWith(input, S_CRLF)){

        int firstIndex;
        if (isGet){  
          firstIndex = StringUtils::flashStringLength(S_WIFI_GET_) - 1;
        } 
        else {
          firstIndex = StringUtils::flashStringLength(S_WIFI_POST_) - 1;
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
          return RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET;
        } 
        else {
          // Post
          //word dataLength0 = dataLength;
          dataLength -= Serial_skipBytesUntil(dataLength, S_CRLFCRLF); // skip HTTP header
          //word dataLength1 = dataLength;
          dataLength -= Serial_readStringUntil(postParams, dataLength, S_CRLF); // read HTTP data;
          // word dataLength2 = dataLength;           
          Serial_skipBytes(dataLength); // skip remaned endings

          if (StringUtils::flashStringEndsWith(postParams, S_CRLF)){
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
          return RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST; 
        }
      } 
      else {
        // Unknown HTTP request type
        Serial_skipBytes(dataLength); // remove all data
        Serial_skipBytes(2); // remove end mark 
        return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
      }

    } 
    else if (firstRequestHeaderByte == 0x80) {
      // TCP client connected
      Serial_readString(input, 1); 
      wifiPortDescriptor = input[14]; 
      Serial_skipBytes(8); 
      return RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_CONNECTED;

    } 
    else if (firstRequestHeaderByte == 0x81) {
      // TCP client disconnected
      Serial_readString(input, 1); 
      wifiPortDescriptor = input[14]; 
      Serial_skipBytes(8); 
      return RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_DISCONNECTED;

    } 
    else if (firstRequestHeaderByte == 0xFF) { 
      // Data received Failed
      Serial_skipBytes(2); // remove end mark and exit quick
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;

    } 
    else {
      // Unknown packet and it size
      RAK410_XBeeWifi.cleanSerialBuffer();
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;   
    }
  }
  return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;  
} 


void RAK410_XBeeWifiClass::sendWifiFrameStart(const byte portDescriptor, word length){ // 1400 bytes max (Wi-Fi module spec restriction)   
  Serial1.print(F("at+send_data="));
  Serial1.print(portDescriptor);
  Serial1.print(',');
  Serial1.print(length);
  Serial1.print(',');
}

void RAK410_XBeeWifiClass::sendWifiFrameData(const __FlashStringHelper* data){
  Serial1.print(data);
}

boolean RAK410_XBeeWifiClass::sendWifiFrameStop(){
  s_restartWifiIfNoResponseAutomatically = false;
  boolean rez = wifiExecuteCommand();
  s_restartWifiIfNoResponseAutomatically = true;
  return rez;
}

void RAK410_XBeeWifiClass::sendWifiData(const byte portDescriptor, const __FlashStringHelper* data){ // INT_MAX (own test) or 1400 bytes max (Wi-Fi spec restriction)
  int length = StringUtils::flashStringLength(data);
  if (length == 0){
    return;
  }
  sendWifiFrameStart(portDescriptor, length);
  Serial1.print(data);
  sendWifiFrameStop();
}

void RAK410_XBeeWifiClass::sendWifiDataStart(const byte &wifiPortDescriptor){
  sendWifiFrameStart(wifiPortDescriptor, WIFI_MAX_SEND_FRAME_SIZE);
  s_sendWifiDataFrameSize = 0;
}





boolean RAK410_XBeeWifiClass::sendWifiAutoFrameData(const byte &wifiPortDescriptor, const __FlashStringHelper* data){
  boolean isSendOK = true;
  if (s_sendWifiDataFrameSize + StringUtils::flashStringLength(data) < WIFI_MAX_SEND_FRAME_SIZE){
    s_sendWifiDataFrameSize += Serial1.print(data);
  } 
  else {
    int index = 0;
    while (s_sendWifiDataFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
      char c = StringUtils::flashStringCharAt(data, index++);
      s_sendWifiDataFrameSize += Serial1.print(c);
    }
    isSendOK = sendWifiDataStop();
    sendWifiDataStart(wifiPortDescriptor);   
    while (index < StringUtils::flashStringLength(data)){
      char c = StringUtils::flashStringCharAt(data, index++);
      s_sendWifiDataFrameSize += Serial1.print(c);
    } 

  }
  return isSendOK;
}  

boolean RAK410_XBeeWifiClass::sendWifiAutoFrameData(const byte &wifiPortDescriptor, const String &data){
  boolean isSendOK = true;
  if (data.length() == 0){
    return isSendOK;
  }
  if (s_sendWifiDataFrameSize + data.length() < WIFI_MAX_SEND_FRAME_SIZE){
    s_sendWifiDataFrameSize += Serial1.print(data);
  } 
  else {
    int index = 0;
    while (s_sendWifiDataFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
      char c = data[index++];
      s_sendWifiDataFrameSize += Serial1.print(c);
    }
    isSendOK = sendWifiDataStop();
    sendWifiDataStart(wifiPortDescriptor); 

    while (index < data.length()){
      char c = data[index++];
      s_sendWifiDataFrameSize += Serial1.print(c);
    }      
  }
  return isSendOK;
}







boolean RAK410_XBeeWifiClass::sendWifiDataStop(){
  if (s_sendWifiDataFrameSize > 0){
    while (s_sendWifiDataFrameSize < WIFI_MAX_SEND_FRAME_SIZE){
      s_sendWifiDataFrameSize += Serial1.write(0x00); // Filler 0x00
    }
  }
  return sendWifiFrameStop();
} 

boolean RAK410_XBeeWifiClass::sendWifiCloseConnection(const byte portDescriptor){
  Serial1.print(F("at+cls="));
  Serial1.print(portDescriptor);
  return wifiExecuteCommand(); 
}




//private:
void RAK410_XBeeWifiClass::showWifiMessage(const __FlashStringHelper* str, boolean newLine){ //TODO 
  if (g_useSerialMonitor){
    Serial.print(FS(S_WIFI));
    Serial.print(str);
    if (newLine){  
      Serial.println();
    }      
  }
}

/////////////////////////////////////////////////////////////////////
//                             Wi-FI DEVICE                        //
/////////////////////////////////////////////////////////////////////

boolean RAK410_XBeeWifiClass:: startWifiSilent(){

  cleanSerialBuffer();

  if (!wifiExecuteCommand(F("at+scan=0"), 5000)){
    return false;
  } 

  boolean isStationMode = (s_wifiSID.length()>0);    
  if (isStationMode){
    if (s_wifiPass.length() > 0){
      Serial1.print(F("at+psk="));
      Serial1.print(s_wifiPass);
      if (!wifiExecuteCommand()){
        return false;
      }
    } 

    Serial1.print(F("at+connect="));
    Serial1.print(s_wifiSID);
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

    // at+ip=<ip>,<mask>,<gateway>,<dns server1>(0 is valid),<dns server2>(0 is valid)\r\n
    if (!wifiExecuteCommand(F("at+ip=192.168.0.1,255.255.0.0,0.0.0.0,0,0"))){
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

boolean RAK410_XBeeWifiClass::wifiExecuteCommand(const __FlashStringHelper* command, int maxResponseDeleay){   
  String input = wifiExecuteRawCommand(command, maxResponseDeleay);
  if (input.length() == 0){
    if (s_restartWifiIfNoResponseAutomatically){
      s_restartWifi = true;
    }

    if (g_useSerialMonitor){   
      showWifiMessage(F("No response"), false);
      if (s_restartWifiIfNoResponseAutomatically){
        Serial.print(F(" (reboot)"));
      } 
      Serial.println();
    }
    // Nothing to do
  } 
  else if (StringUtils::flashStringStartsWith(input, S_WIFI_RESPONSE_OK) && StringUtils::flashStringEndsWith(input, S_CRLF)){
    return true;
  } 
  else if (StringUtils::flashStringStartsWith(input, S_WIFI_RESPONSE_ERROR) && StringUtils::flashStringEndsWith(input, S_CRLF)){
    if (g_useSerialMonitor){
      byte errorCode = input[5];
      showWifiMessage(F("Error "), false);
      Serial.print(StringUtils::getHEX(errorCode, true));
      Serial.println();
    }      
  } 
  else {
    if (g_useSerialMonitor){
      showWifiMessage(FS(S_empty), false);
      PrintUtils::printWithoutCRLF(input);
      Serial.print(FS(S_Next));
      PrintUtils::printHEX(input); 
      Serial.println();
    }
  }

  return false;
}

String RAK410_XBeeWifiClass::wifiExecuteRawCommand(const __FlashStringHelper* command, int maxResponseDeleay){

  cleanSerialBuffer();

  if (command == 0){
    Serial1.println();
  } 
  else {
    Serial1.println(command);
  }

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

/////////////////////////////////////////////////////////////////////
//                          SERIAL READ                            //
/////////////////////////////////////////////////////////////////////

// WARNING! This is adapted copy of Stream.h, Serial.h, and HardwareSerial.h
// functionality
boolean RAK410_XBeeWifiClass::Serial_timedRead(char* c){
  unsigned long _startMillis = millis();
  unsigned long _currentMillis;
  do {
    if (Serial1.available()){
      *c = (char) Serial1.read();
      return true;   
    }
    _currentMillis = millis();
  } 
  while(((_currentMillis - _startMillis) < Stream_timeout) || (_currentMillis < _startMillis));  // Overflow check 
  //while((_currentMillis - _startMillis) < Stream_timeout); 
  //while(millis() - _startMillis < Stream_timeout); 
  return false;     // false indicates timeout
}

//public:
size_t RAK410_XBeeWifiClass::Serial_readBytes(char *buffer, size_t length) {
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

size_t RAK410_XBeeWifiClass::Serial_skipBytes(size_t length) {
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

size_t RAK410_XBeeWifiClass::Serial_skipBytesUntil(size_t length, const char PROGMEM* pstr){   
  int pstr_length = StringUtils::flashStringLength(pstr);   
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
    if (count >= pstr_length && StringUtils::flashStringEquals(matcher, pstr_length, pstr)){
      break;
    } 
  }
  return count;
}  

size_t RAK410_XBeeWifiClass::Serial_readStringUntil(String& str, size_t length, const char PROGMEM* pstr){      
  char c;
  size_t count = 0;
  while (count < length) {
    if (!Serial_timedRead(&c)){
      break;
    }
    count++;
    str +=c;
    if (StringUtils::flashStringEndsWith(str, pstr)){
      break;
    } 
  }
  return count;
} 

size_t RAK410_XBeeWifiClass::Serial_readString(String& str, size_t length){
  char buffer[length];
  size_t count = Serial_readBytes(buffer, length);
  str.reserve(str.length() + count);
  for (size_t i = 0; i < count; i++) {
    str += buffer[i];  
  }
  return count;
}

size_t RAK410_XBeeWifiClass::Serial_readString(String& str){

  size_t maxFrameLenght = 100; 
  size_t countInFrame = Serial_readString(str, maxFrameLenght);

  size_t count = countInFrame; 

  while (countInFrame == maxFrameLenght){
    countInFrame = Serial_readString(str, maxFrameLenght); 
    count += countInFrame;
  }
  return count;
}

RAK410_XBeeWifiClass RAK410_XBeeWifi;

