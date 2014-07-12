#ifndef WebServer_h
#define WebServer_h

#include <MemoryFree.h>

#include "Global.h"
#include "StorageHelper.h" 
#include "SerialHelper.h" 
#include "Thermometer.h" 

/////////////////////////////////////////////////////////////////////
//                           HTML CONSTS                           //
/////////////////////////////////////////////////////////////////////
const char S_hr[] PROGMEM  = "hr";
const char S_br[] PROGMEM  = "br";
const char S_table[] PROGMEM  = "table";
const char S_tr[] PROGMEM  = "tr";
const char S_td[] PROGMEM  = "td";
const char S_pre[] PROGMEM  = "pre";
const char S_html[] PROGMEM  = "html";
const char S_h1[] PROGMEM  = "h1";

const char S_url[] PROGMEM  = "/";
const char S_url_log[] PROGMEM  = "/log";
const char S_url_conf[] PROGMEM  = "/conf";
const char S_url_storage[] PROGMEM  = "/storage";

class GB_WebServer{
private:  
  // HTTP response supplemental   
  GB_COMMAND_TYPE c_commandType;

  byte c_wifiPortDescriptor;
  byte c_isWifiResponseError;

public:  
  GB_WebServer() : 
  c_commandType(GB_COMMAND_NONE), c_wifiPortDescriptor(0xFF), c_isWifiResponseError(false){
  }


  void handleSerialEvent(){

    String input;   
    String postParams; 

    c_commandType = handleSerialEvent(input, c_wifiPortDescriptor, postParams);

    //  Serial.print(F("WIFI > input: "));
    //  Serial.print(input);
    //  Serial.print(F(" post: "));
    //  Serial.print(postParams);

    switch(c_commandType){
    case GB_COMMAND_HTTP_POST:
      sendHTTPRedirect(c_wifiPortDescriptor, FS(S_url));
      break;

    case GB_COMMAND_HTTP_GET:
      if (StringUtils::flashStringEquals(input, S_url) || 
          StringUtils::flashStringEquals(input, S_url_log) ||
          StringUtils::flashStringEquals(input, S_url_conf) ||
          StringUtils::flashStringEquals(input, S_url_storage)){

        sendHttpOK_Header(c_wifiPortDescriptor);

        generateHttpResponsePage(input);

        sendHttpOK_PageComplete(c_wifiPortDescriptor);

        if(GB_SerialHelper::useSerialMonitor){ 
          if (c_isWifiResponseError){
            Serial.print(FS(S_WIFI));
            Serial.println(F("Send responce error"));
            GB_SerialHelper::printDirtyEnd();
          }
        }
      } 
      else {
        // Unknown resource
        sendHttpNotFound(c_wifiPortDescriptor);    
      }
      break;
    }
  }

private:

  static GB_COMMAND_TYPE handleSerialEvent(String &input, byte &wifiPortDescriptor, String &postParams){

    input = String();
    input.reserve(100);
    wifiPortDescriptor = 0xFF;
    postParams = String();

    GB_SerialHelper::Serial_readString(input, 13); // "at+recv_data="

    if (!StringUtils::flashStringEquals(input, F("at+recv_data="))){
      // Read data from serial manager
      GB_SerialHelper::Serial_readString(input); // at first we should read, after manipulate  

      if (StringUtils::flashStringStartsWith(input, S_WIFI_RESPONSE_WELLCOME) || StringUtils::flashStringStartsWith(input, S_WIFI_RESPONSE_ERROR)){
        GB_SerialHelper::checkSerial(false, true); // manual restart, or wrong state of Wi-Fi
        return GB_COMMAND_NONE;
      }

      return GB_COMMAND_SERIAL_MONITOR;
    } 
    else {
      // WARNING! We need to do it quick. Standart serial buffer capacity only 64 bytes
      GB_SerialHelper::Serial_readString(input, 1); // ends with '\r', cause '\n' will be removed
      byte firstRequestHeaderByte = input[13]; //
      
      if (firstRequestHeaderByte <= 0x07) {        
        // Data Received Successfully
        wifiPortDescriptor = firstRequestHeaderByte; 
    
        GB_SerialHelper::Serial_readString(input, 8);  // get full request header

        byte lowByteDataLength = input[20];
        byte highByteDataLength = input[21];
        word dataLength = (((word)highByteDataLength) << 8) + lowByteDataLength;

        // Check HTTP type 
        input = String();
        input.reserve(100);
        dataLength -= GB_SerialHelper::Serial_readStringUntil(input, dataLength, S_CRLF);

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
            GB_SerialHelper::Serial_skipBytes(dataLength); 
            GB_SerialHelper::Serial_skipBytes(2); // remove end mark 
            return GB_COMMAND_HTTP_GET;
          } 
          else {
            // Post
            //word dataLength0 = dataLength;
            dataLength -= GB_SerialHelper::Serial_skipBytesUntil(dataLength, S_CRLFCRLF); // skip HTTP header
            //word dataLength1 = dataLength;
            dataLength -= GB_SerialHelper::Serial_readStringUntil(postParams, dataLength, S_CRLF); // read HTTP data;
            // word dataLength2 = dataLength;           
            GB_SerialHelper::Serial_skipBytes(dataLength); // skip remaned endings

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
            GB_SerialHelper::Serial_skipBytes(2); // remove end mark 
            return GB_COMMAND_HTTP_POST; 
          }
        } 
        else {
          // Unknown HTTP request type
          GB_SerialHelper::Serial_skipBytes(dataLength); // remove all data
          GB_SerialHelper::Serial_skipBytes(2); // remove end mark 
          return GB_COMMAND_NONE;
        }

      } 
      else if (firstRequestHeaderByte == 0x80) {
        // TCP client connected
        GB_SerialHelper::Serial_readString(input, 1); 
        wifiPortDescriptor = input[14]; 
        GB_SerialHelper::Serial_skipBytes(8); 
        return GB_COMMAND_HTTP_CONNECTED;

      } 
      else if (firstRequestHeaderByte == 0x81) {
        // TCP client disconnected
        GB_SerialHelper::Serial_readString(input, 1); 
        wifiPortDescriptor = input[14]; 
        GB_SerialHelper::Serial_skipBytes(8); 
        return GB_COMMAND_HTTP_DISCONNECTED;

      } 
      else if (firstRequestHeaderByte == 0xFF) { 
        // Data received Failed
        GB_SerialHelper::Serial_skipBytes(2); // remove end mark and exit quick
        return GB_COMMAND_NONE;

      } 
      else {
        // Unknown packet and it size
        GB_SerialHelper::cleanSerialBuffer();
        return GB_COMMAND_NONE;   
      }
    }
    return GB_COMMAND_NONE;  
  } 

  /////////////////////////////////////////////////////////////////////
  //                           HTTP PROTOCOL                         //
  /////////////////////////////////////////////////////////////////////

  static void sendHttpNotFound(const byte wifiPortDescriptor){ 
    GB_SerialHelper::sendWifiData(wifiPortDescriptor, F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
    GB_SerialHelper::sendWifiCloseConnection(wifiPortDescriptor);
  }
  
  // WARNING! RAK 410 became mad when 2 parallel connections comes. Like with Chrome and POST request, when RAK response 303.
  // Connection for POST request closed by Chrome (not by RAK). And during this time Chrome creates new parallel connection for GET
  // request.
  static void sendHTTPRedirect(const byte &wifiPortDescriptor, const __FlashStringHelper* data){ 
    //const __FlashStringHelper* header = F("HTTP/1.1 303 See Other\r\nLocation: "); // DO not use it with RAK 410
    const __FlashStringHelper* header = F("HTTP/1.1 200 OK (303 doesn't work on RAK 410)\r\nrefresh: 0; url="); 
    
    GB_SerialHelper::sendWifiFrameStart(wifiPortDescriptor, StringUtils::flashStringLength(header) + StringUtils::flashStringLength(data) + StringUtils::flashStringLength(S_CRLFCRLF));
    
    GB_SerialHelper::sendWifiFrameData(header);
    GB_SerialHelper::sendWifiFrameData(data);
    GB_SerialHelper::sendWifiFrameData(FS(S_CRLFCRLF));
    
    GB_SerialHelper::sendWifiFrameStop();
    
    GB_SerialHelper::sendWifiCloseConnection(wifiPortDescriptor);
  }

  static void sendHttpOK_Header(const byte wifiPortDescriptor){ 
    GB_SerialHelper::sendWifiData(wifiPortDescriptor, F("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"));
    GB_SerialHelper::sendWifiDataStart(wifiPortDescriptor);
  }

  static void sendHttpOK_PageComplete(const byte &wifiPortDescriptor){  
    GB_SerialHelper::sendWifiDataStop();
    GB_SerialHelper::sendWifiCloseConnection(wifiPortDescriptor);
  }



  static void showWifiMessage(const __FlashStringHelper* str, boolean newLine = true){ //TODO 
    if (GB_SerialHelper::useSerialMonitor){
      Serial.print(FS(S_WIFI));
      Serial.print(str);
      if (newLine){  
        Serial.println();    
      }      
    }
  }

 
  /////////////////////////////////////////////////////////////////////
  //                     HTTP SUPPLEMENTAL COMMANDS                  //
  /////////////////////////////////////////////////////////////////////

  void sendData(const __FlashStringHelper* data){
    if (c_commandType == GB_COMMAND_HTTP_GET){
      if (!GB_SerialHelper::sendWifiAutoFrameData(c_wifiPortDescriptor, data)){
        c_isWifiResponseError = true;
      }
    } 
    else {
      Serial.print(data); 
    }
  }

  void sendData(const String &data){
    if (c_commandType == GB_COMMAND_HTTP_GET){
      if (!GB_SerialHelper::sendWifiAutoFrameData(c_wifiPortDescriptor, data)){
        c_isWifiResponseError = true;
      }
    } 
    else {
      Serial.print(data); 
    }
  }

  void sendDataLn(){
    sendData(FS(S_CRLF));
  }

  /// TODO optimize it
  void sendData(int data){
    String str; 
    str += data;
    sendData(str);
  }

  void sendData(word data){
    String str; 
    str += data;
    sendData(str);
  }

  void sendData(char data){
    String str; 
    str += data;
    sendData(str);
  }

  void sendData(float data){
    String str = GB_PrintDirty::floatToString(data);
    sendData(str);
  }

  void sendData(time_t data){
    String str = GB_PrintDirty::getTimeString(data);
    sendData(str);
  }


  void sendTagButton(const char PROGMEM* url, const __FlashStringHelper* name){
    sendData(F("<input type=\"button\" onclick=\"document.location='"));
    sendData(FS(url));
    sendData(F("'\" value=\""));
    sendData(name);
    sendData(F("\"/>"));
  }

  void sendTag_Begin(HTTP_TAG type){
    sendData('<');
    if (type == HTTP_TAG_CLOSED){
      sendData('/');
    }
  }

  void sendTag_End(HTTP_TAG type){
    if (type == HTTP_TAG_SINGLE){
      sendData('/');
    }
    sendData('>');
  }

  void sendTag(const char PROGMEM* pname, HTTP_TAG type){
    sendTag_Begin(type);
    sendData(FS(pname));
    sendTag_End(type);
  }

  void sendTag(const char tag, HTTP_TAG type){
    sendTag_Begin(type);
    sendData(tag);
    sendTag_End(type);
  }

  void generateHttpResponsePage(const String &input){

    if (c_commandType == GB_COMMAND_HTTP_GET){
      sendTag(S_html, HTTP_TAG_OPEN); 
      sendTag(S_h1, HTTP_TAG_OPEN); 
      sendData(F("Growbox"));  
      sendTag(S_h1, HTTP_TAG_CLOSED);
      sendTagButton(S_url, F("Status"));
      sendTagButton(S_url_log, F("Daily log"));
      sendTagButton(S_url_conf, F("Configuration"));
      sendTagButton(S_url_storage, F("Storage dump"));
      sendTag(S_hr, HTTP_TAG_SINGLE);
      sendTag(S_pre, HTTP_TAG_OPEN);
      sendBriefStatus();
      sendTag(S_pre, HTTP_TAG_CLOSED);
    }

    sendTag(S_pre, HTTP_TAG_OPEN);
    if (StringUtils::flashStringEquals(input, S_url)){
      printSendPinsStatus();   
    } 
    else if (StringUtils::flashStringEquals(input, S_url_conf)){
      printSendConfigurationControls(); 
    } 
    else if (StringUtils::flashStringEquals(input, S_url_log)){
      printSendFullLog(true, true, true); // TODO use parameters
    }
    else if (StringUtils::flashStringEquals(input, S_url_storage)){
      sendStorageDump(); 
    }

    if (c_isWifiResponseError) return;

    sendTag(S_pre, HTTP_TAG_CLOSED);
    sendTag(S_html, HTTP_TAG_CLOSED);
    /*
    // read the incoming byte:
     char firstChar = 0, secondChar = 0; 
     firstChar = input[0];
     if (input.length() > 1){
     secondChar = input[1];
     }
     
     Serial.print(F("COMMAND>"));
     Serial.print(firstChar);
     if (secondChar != 0){
     Serial.print(secondChar);
     }
     Serial.println();
     
     boolean printEvents = true; // can't put in switch, Arduino bug
     boolean printErrors = true;
     boolean printTemperature = true;
     
     switch(firstChar){
     case 's':
     printprintSendFullStatus(); 
     break;  
     
     case 'l':
     switch(secondChar){
     case 'c': 
     Serial.println(F("Stored log records cleaned"));
     GB_StorageHelper::resetStoredLog();
     break;
     case 'e':
     Serial.println(F("Logger store enabled"));
     GB_StorageHelper::setStoreLogRecordsEnabled(true);
     break;
     case 'd':
     Serial.println(F("Logger store disabled"));
     GB_StorageHelper::setStoreLogRecordsEnabled(false);
     break;
     case 't': 
     printErrors = printEvents = false;
     break;
     case 'r': 
     printEvents = printTemperature = false;
     break;
     case 'v': 
     printTemperature = printErrors = false;
     break;
     } 
     if ((secondChar != 'c') && (secondChar != 'e') && (secondChar != 'd')){
     GB_LOGGER.printFullLog(printEvents,  printErrors,  printTemperature );
     }
     break; 
     
     case 'b': 
     switch(secondChar){
     case 'c': 
     Serial.println(F("Cleaning boot record"));
     
     GB_StorageHelper::resetFirmware();
     Serial.println(F("Magic number corrupted, reseting"));
     
     Serial.println('5');
     delay(1000);
     Serial.println('4');
     delay(1000);
     Serial.println('3');
     delay(1000);
     Serial.println('2');
     delay(1000);
     Serial.println('1');
     delay(1000);
     Serial.println(F("Rebooting..."));
     GB_Controller::rebootController();
     break;
     } 
     
     Serial.println(F("Currnet boot record"));
     Serial.print(F("-Memory : ")); 
     {// TODO compilator madness
     BootRecord bootRecord = GB_StorageHelper::getBootRecord();
     GB_PrintDirty::printRAM(&bootRecord, sizeof(BootRecord));
     Serial.println();
     }
     Serial.print(F("-Storage: ")); 
     printSendStorage(0, sizeof(BootRecord));
     
     break;  
     
     case 'm':    
     switch(secondChar){
     case '0': 
     AT24C32_EEPROM.fillStorage(0x00); 
     break; 
     case 'a': 
     AT24C32_EEPROM.fillStorage(0xAA); 
     break; 
     case 'f': 
     AT24C32_EEPROM.fillStorage(0xFF); 
     break; 
     case 'i': 
     AT24C32_EEPROM.fillStorageIncremental(); 
     break; 
     }
     printSendStorage();
     break; 
     case 'r':        
     Serial.println(F("Rebooting..."));
     GB_Controller::rebootController();
     break; 
     default: 
     GB_LOGGER.logEvent(EVENT_SERIAL_UNKNOWN_COMMAND);  
     }
     */

  }

  void sendBriefStatus(){
    sendFreeMemory();
    sendBootStatus();
    sendTimeStatus();
    sendTemperatureStatus();  
    sendTag(S_hr, HTTP_TAG_SINGLE); 
  }


  void printSendConfigurationControls(){
    sendData(F("<form action=\"/\" method=\"post\">"));
    sendData(F("<input type=\"submit\" value=\"Submit\">"));
    sendData(F("</form>"));
  }

  void sendFreeMemory(){  
    sendData(FS(S_Free_memory));
    //sendTab_B(HTTP_TAG_OPEN);
    sendData(freeMemory()); 
    sendData(FS(S_bytes));
    sendDataLn();
  }

  // private:

  void sendBootStatus(){
    sendData(F("Controller: startup: ")); 
    sendData(GB_StorageHelper::getLastStartupTimeStamp());
    sendData(F(", first startup: ")); 
    sendData(GB_StorageHelper::getFirstStartupTimeStamp());
    sendData(F("\r\nLogger:"));
    if (GB_StorageHelper::isStoreLogRecordsEnabled()){
      sendData(FS(S_enabled));
    } 
    else {
      sendData(FS(S_disabled));
    }
    sendData(F(", records "));
    sendData(GB_StorageHelper::getLogRecordsCount());
    sendData('/');
    sendData(GB_StorageHelper::LOG_CAPACITY);
    if (GB_StorageHelper::isLogOverflow()){
      sendData(F(", overflow"));
    } 
    sendDataLn();
  }

  void sendTimeStatus(){
    sendData(F("Clock: ")); 
    sendTag('b', HTTP_TAG_OPEN);
    if (g_isDayInGrowbox) {
      sendData(F("DAY"));
    } 
    else{
      sendData(F("NIGHT"));
    }
    sendTag('b', HTTP_TAG_CLOSED);
    sendData(F(" mode, time ")); 
    sendData(now());
    sendData(F(", up time [")); 
    sendData(UP_HOUR); 
    sendData(F(":00], down time ["));
    sendData(DOWN_HOUR);
    sendData(F(":00]\r\n"));
  }

  void sendTemperatureStatus(){
    float workingTemperature, statisticsTemperature;
    int statisticsCount;
    GB_Thermometer::getStatistics(workingTemperature, statisticsTemperature, statisticsCount);

    sendData(FS(S_Temperature)); 
    sendData(F(": current ")); 
    sendData(workingTemperature);
    sendData(F(", next ")); 
    sendData(statisticsTemperature);
    sendData(F(" (count ")); 
    sendData(statisticsCount);

    sendData(F("), day "));
    sendData(TEMPERATURE_DAY);
    sendData(FS(S_PlusMinus));
    sendData(TEMPERATURE_DELTA);
    sendData(F(", night "));
    sendData(TEMPERATURE_NIGHT);
    sendData(FS(S_PlusMinus));
    sendData(2*TEMPERATURE_DELTA);
    sendData(F(", critical "));
    sendData(TEMPERATURE_CRITICAL);
    sendDataLn();
  }

  void printSendPinsStatus(){
    sendData(F("Pin OUTPUT INPUT")); 
    sendDataLn();
    for(int i=0; i<=19;i++){
      sendData(' ');
      if (i>=14){
        sendData('A');
        sendData(i-14);
      } 
      else { 
        sendData(GB_PrintDirty::getFixedDigitsString(i, 2));
      }
      sendData(FS(S___)); 

      boolean io_status, dataStatus, inputStatus;
      if (i<=7){ 
        io_status = bitRead(DDRD, i);
        dataStatus = bitRead(PORTD, i);
        inputStatus = bitRead(PIND, i);
      }    
      else if (i <= 13){
        io_status = bitRead(DDRB, i-8);
        dataStatus = bitRead(PORTB, i-8);
        inputStatus = bitRead(PINB, i-8);
      }
      else {
        io_status = bitRead(DDRC, i-14);
        dataStatus = bitRead(PORTC, i-14);
        inputStatus = bitRead(PINC, i-14);
      }
      if (io_status == OUTPUT){
        sendData(FS(S___));
        sendData(dataStatus);
        sendData(F("     -   "));
      } 
      else {
        sendData(F("  -     "));
        sendData(inputStatus);
        sendData(FS(S___));
      }

      switch(i){
      case 0: 
      case 1: 
        sendData(F("Reserved by Serial/USB. Can be used, if Serial/USB won't be connected"));
        break;
      case LIGHT_PIN: 
        sendData(F("Relay: light on(0)/off(1)"));
        break;
      case FAN_PIN: 
        sendData(F("Relay: fan on(0)/off(1)"));
        break;
      case FAN_SPEED_PIN: 
        sendData(F("Relay: fun max(0)/min(1) speed switch"));
        break;
      case ONE_WIRE_PIN: 
        sendData(F("1-Wire: termometer"));
        break;
      case USE_SERIAL_MONOTOR_PIN: 
        sendData(F("Use serial monitor on(1)/off(0)"));
        break;
      case ERROR_PIN: 
        sendData(F("Error status"));
        break;
      case BREEZE_PIN: 
        sendData(F("Breeze"));
        break;
      case 18: 
      case 19: 
        sendData(F("Reserved by I2C. Can be used, if SCL, SDA pins will be used"));
        break;
      }
      sendDataLn();
    }
  }


  void printSendFullLog(boolean printEvents, boolean printErrors, boolean printTemperature){
    LogRecord logRecord;
    boolean isEmpty = true;
    sendTag(S_table, HTTP_TAG_OPEN);
    for (int i = 0; i < GB_LOGGER.getLogRecordsCount(); i++){

      logRecord = GB_LOGGER.getLogRecordByIndex(i);
      if (!printEvents && GB_LOGGER.isEvent(logRecord)){
        continue;
      }
      if (!printErrors && GB_LOGGER.isError(logRecord)){
        continue;
      }
      if (!printTemperature && GB_LOGGER.isTemperature(logRecord)){
        continue;
      }

      sendTag(S_tr, HTTP_TAG_OPEN);
      sendTag(S_td, HTTP_TAG_OPEN);
      sendData(i+1);
      sendTag(S_td, HTTP_TAG_CLOSED);
      sendTag(S_td, HTTP_TAG_OPEN);
      sendData(GB_PrintDirty::getTimeString(logRecord.timeStamp));    
      sendTag(S_td, HTTP_TAG_CLOSED);
      sendTag(S_td, HTTP_TAG_OPEN);
      sendData(GB_PrintDirty::getHEX(logRecord.data, true));
      sendTag(S_td, HTTP_TAG_CLOSED);
      sendTag(S_td, HTTP_TAG_OPEN);
      sendData(GB_LOGGER.getLogRecordDescription(logRecord));
      sendData(GB_LOGGER.getLogRecordSuffix(logRecord));
      sendTag(S_td, HTTP_TAG_CLOSED);
      //sendDataLn();
      sendTag(S_tr, HTTP_TAG_CLOSED);
      isEmpty = false;

      if (c_isWifiResponseError) return;

    }
    sendTag(S_table, HTTP_TAG_CLOSED);
    if (isEmpty){
      sendData(F("Log empty"));
    }
  }

  // TODO garbage?
  void printStorage(word address, byte sizeOf){
    byte buffer[sizeOf];
    AT24C32_EEPROM.read(address, buffer, sizeOf);
    GB_PrintDirty::printRAM(buffer, sizeOf);
    Serial.println();
  }

  void sendStorageDump(){
    sendTag(S_table, HTTP_TAG_OPEN);
    sendTag(S_tr, HTTP_TAG_OPEN);
    sendTag(S_td, HTTP_TAG_OPEN);
    sendTag(S_td, HTTP_TAG_CLOSED);
    for (word i = 0; i < 16 ; i++){
      sendTag(S_td, HTTP_TAG_OPEN);
      sendTag('b', HTTP_TAG_OPEN);
      sendData(GB_PrintDirty::getHEX(i));
      sendTag('b', HTTP_TAG_CLOSED); 
      sendTag(S_td, HTTP_TAG_CLOSED);
    }
    sendTag(S_tr, HTTP_TAG_CLOSED);

    for (word i = 0; i < AT24C32_EEPROM.CAPACITY ; i++){
      byte value = AT24C32_EEPROM.read(i);

      if (i% 16 ==0){
        if (i>0){
          sendTag(S_tr, HTTP_TAG_CLOSED);
        }
        sendTag(S_tr, HTTP_TAG_OPEN);
        sendTag(S_td, HTTP_TAG_OPEN);
        sendTag('b', HTTP_TAG_OPEN);
        sendData(GB_PrintDirty::getHEX(i/16));
        sendTag('b', HTTP_TAG_CLOSED);
        sendTag(S_td, HTTP_TAG_CLOSED);
      }
      sendTag(S_td, HTTP_TAG_OPEN);
      sendData(GB_PrintDirty::getHEX(value));
      sendTag(S_td, HTTP_TAG_CLOSED);

      if (c_isWifiResponseError) return;

    }
    sendTag(S_tr, HTTP_TAG_CLOSED);
    sendTag(S_table, HTTP_TAG_CLOSED);
  }

};

extern GB_WebServer webServer;
#endif


