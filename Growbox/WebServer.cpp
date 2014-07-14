#include "WebServer.h"

#include "RAK410_XBeeWifi.h" 

void WebServerClass::handleSerialEvent(){

  String input;   
  String postParams; 

  // HTTP response supplemental   
  RAK410_XBeeWifiClass::RequestType c_commandType = RAK410_XBeeWifi.handleSerialEvent(c_wifiPortDescriptor, input, postParams);

  //  Serial.print(F("WIFI > input: "));
  //  Serial.print(input);
  //  Serial.print(F(" post: "));
  //  Serial.print(postParams);

  switch(c_commandType){
  case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST:
    sendHTTPRedirect(c_wifiPortDescriptor, FS(S_url));
    break;

  case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET:
    if (StringUtils::flashStringEquals(input, FS(S_url)) || 
      StringUtils::flashStringEquals(input, FS(S_url_log)) ||
      StringUtils::flashStringEquals(input, FS(S_url_conf)) ||
      StringUtils::flashStringEquals(input, FS(S_url_storage))){

      sendHttpOK_Header(c_wifiPortDescriptor);

      generateHttpResponsePage(input);

      sendHttpOK_PageComplete(c_wifiPortDescriptor);

      if(g_useSerialMonitor){ 
        if (c_isWifiResponseError){
          showWebMessage(F("Send responce error"));
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


/////////////////////////////////////////////////////////////////////
//                           HTTP PROTOCOL                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendHttpNotFound(const byte wifiPortDescriptor){ 
  RAK410_XBeeWifi.sendFixedSizeData(wifiPortDescriptor, F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
  RAK410_XBeeWifi.sendCloseConnection(wifiPortDescriptor);
}

// WARNING! RAK 410 became mad when 2 parallel connections comes. Like with Chrome and POST request, when RAK response 303.
// Connection for POST request closed by Chrome (not by RAK). And during this time Chrome creates new parallel connection for GET
// request.
void WebServerClass::sendHTTPRedirect(const byte &wifiPortDescriptor, const __FlashStringHelper* data){ 
  //const __FlashStringHelper* header = F("HTTP/1.1 303 See Other\r\nLocation: "); // DO not use it with RAK 410
  const __FlashStringHelper* header = F("HTTP/1.1 200 OK (303 doesn't work on RAK 410)\r\nrefresh: 0; url="); 

  RAK410_XBeeWifi.sendFixedSizeFrameStart(wifiPortDescriptor, StringUtils::flashStringLength(header) + StringUtils::flashStringLength(data) + StringUtils::flashStringLength(FS(S_CRLFCRLF)));

  RAK410_XBeeWifi.sendFixedSizeFrameData(header);
  RAK410_XBeeWifi.sendFixedSizeFrameData(data);
  RAK410_XBeeWifi.sendFixedSizeFrameData(FS(S_CRLFCRLF));

  RAK410_XBeeWifi.sendFixedSizeFrameStop();

  RAK410_XBeeWifi.sendCloseConnection(wifiPortDescriptor);
}

void WebServerClass::sendHttpOK_Header(const byte wifiPortDescriptor){ 
  RAK410_XBeeWifi.sendFixedSizeData(wifiPortDescriptor, F("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"));
  RAK410_XBeeWifi.sendAutoSizeFrameStart(wifiPortDescriptor);
}

void WebServerClass::sendHttpOK_PageComplete(const byte &wifiPortDescriptor){  
  RAK410_XBeeWifi.sendAutoSizeFrameStop();
  RAK410_XBeeWifi.sendCloseConnection(wifiPortDescriptor);
}



void WebServerClass::showWebMessage(const __FlashStringHelper* str, boolean newLine){ //TODO 
  if (g_useSerialMonitor){
    Serial.print(FS("WEB> "));
    Serial.print(str);
    if (newLine){  
      Serial.println();    
    }      
  }
}


/////////////////////////////////////////////////////////////////////
//                     HTTP SUPPLEMENTAL COMMANDS                  //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendData(const __FlashStringHelper* data){
    if (!RAK410_XBeeWifi.sendAutoSizeFrameData(c_wifiPortDescriptor, data)){
      c_isWifiResponseError = true;
    }
  } 


void WebServerClass::sendData(const String &data){
    if (!RAK410_XBeeWifi.sendAutoSizeFrameData(c_wifiPortDescriptor, data)){
      c_isWifiResponseError = true;
    }
}

void WebServerClass::sendDataLn(){
  sendData(FS(S_CRLF));
}

/// TODO optimize it
void WebServerClass::sendData(int data){
  String str; 
  str += data;
  sendData(str);
}

void WebServerClass::sendData(word data){
  String str; 
  str += data;
  sendData(str);
}

void WebServerClass::sendData(char data){
  String str; 
  str += data;
  sendData(str);
}

void WebServerClass::sendData(float data){
  String str = StringUtils::floatToString(data);
  sendData(str);
}

void WebServerClass::sendData(time_t data){
  String str = StringUtils::timeToString(data);
  sendData(str);
}


void WebServerClass::sendTagButton(const __FlashStringHelper* url, const __FlashStringHelper* name){
  sendData(F("<input type=\"button\" onclick=\"document.location='"));
  sendData(url);
  sendData(F("'\" value=\""));
  sendData(name);
  sendData(F("\"/>"));
}

void WebServerClass::sendTag_Begin(HTTP_TAG type){
  sendData('<');
  if (type == HTTP_TAG_CLOSED){
    sendData('/');
  }
}

void WebServerClass::sendTag_End(HTTP_TAG type){
  if (type == HTTP_TAG_SINGLE){
    sendData('/');
  }
  sendData('>');
}

void WebServerClass::sendTag(const __FlashStringHelper* pname, HTTP_TAG type){
  sendTag_Begin(type);
  sendData(pname);
  sendTag_End(type);
}

void WebServerClass::sendTag(const char tag, HTTP_TAG type){
  sendTag_Begin(type);
  sendData(tag);
  sendTag_End(type);
}

void WebServerClass::generateHttpResponsePage(const String &input){


    sendTag(FS(S_html), HTTP_TAG_OPEN); 
    sendTag(FS(S_h1), HTTP_TAG_OPEN); 
    sendData(F("Growbox"));  
    sendTag(FS(S_h1), HTTP_TAG_CLOSED);
    sendTagButton(FS(S_url), F("Status"));
    sendTagButton(FS(S_url_log), F("Daily log"));
    sendTagButton(FS(S_url_conf), F("Configuration"));
    sendTagButton(FS(S_url_storage), F("Storage dump"));
    sendTag(FS(S_hr), HTTP_TAG_SINGLE);
    sendTag(FS(S_pre), HTTP_TAG_OPEN);
    sendBriefStatus();
    sendTag(FS(S_pre), HTTP_TAG_CLOSED);

  sendTag(FS(S_pre), HTTP_TAG_OPEN);
  if (StringUtils::flashStringEquals(input, FS(S_url))){
    sendPinsStatus();   
  } 
  else if (StringUtils::flashStringEquals(input, FS(S_url_conf))){
    sendConfigurationControls();
  } 
  else if (StringUtils::flashStringEquals(input, FS(S_url_log))){
    printSendFullLog(true, true, true); // TODO use parameters
  }
  else if (StringUtils::flashStringEquals(input, FS(S_url_storage))){
    sendStorageDump(); 
  }

  if (c_isWifiResponseError) return;

  sendTag(FS(S_pre), HTTP_TAG_CLOSED);
  sendTag(FS(S_html), HTTP_TAG_CLOSED);
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
   GB_StorageHelper.resetStoredLog();
   break;
   case 'e':
   Serial.println(F("Logger store enabled"));
   GB_StorageHelper.setStoreLogRecordsEnabled(true);
   break;
   case 'd':
   Serial.println(F("Logger store disabled"));
   GB_StorageHelper.setStoreLogRecordsEnabled(false);
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
   GB_Logger.printFullLog(printEvents,  printErrors,  printTemperature );
   }
   break; 
   
   case 'b': 
   switch(secondChar){
   case 'c': 
   Serial.println(F("Cleaning boot record"));
   
   GB_StorageHelper.resetFirmware();
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
   BootRecord bootRecord = GB_StorageHelper.getBootRecord();
   GB_PrintUtils.printRAM(&bootRecord, sizeof(BootRecord));
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
   GB_Logger.logEvent(EVENT_SERIAL_UNKNOWN_COMMAND);  
   }
   */

}

void WebServerClass::sendBriefStatus(){
  sendFreeMemory();
  sendBootStatus();
  sendTimeStatus();
  sendTemperatureStatus();  
  sendTag(FS(S_hr), HTTP_TAG_SINGLE); 
}


void WebServerClass::sendConfigurationControls(){
  sendData(F("<form action=\"/\" method=\"post\">"));
  sendData(F("<input type=\"submit\" value=\"Submit\">"));
  sendData(F("</form>"));
}

void WebServerClass::sendFreeMemory(){  
  sendData(FS(S_Free_memory));
  //sendTab_B(HTTP_TAG_OPEN);
  sendData(freeMemory()); 
  sendData(FS(S_bytes));
  sendDataLn();
}

// private:

void WebServerClass::sendBootStatus(){
  sendData(F("Controller: startup: ")); 
  sendData(GB_StorageHelper.getLastStartupTimeStamp());
  sendData(F(", first startup: ")); 
  sendData(GB_StorageHelper.getFirstStartupTimeStamp());
  sendData(F("\r\nLogger: "));
  if (GB_StorageHelper.isStoreLogRecordsEnabled()){
    sendData(FS(S_Enabled));
  } 
  else {
    sendData(FS(S_Disabled));
  }
  sendData(F(", records "));
  sendData(GB_StorageHelper.getLogRecordsCount());
  sendData('/');
  sendData(GB_StorageHelper.LOG_CAPACITY);
  if (GB_StorageHelper.isLogOverflow()){
    sendData(F(", overflow"));
  } 
  sendDataLn();
}

void WebServerClass::sendTimeStatus(){
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

void WebServerClass::sendTemperatureStatus(){
  float workingTemperature, statisticsTemperature;
  int statisticsCount;
  GB_Thermometer.getStatistics(workingTemperature, statisticsTemperature, statisticsCount);

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

void WebServerClass::sendPinsStatus(){
  sendData(F("Pin OUTPUT INPUT")); 
  sendDataLn();
  for(int i=0; i<=19;i++){
    sendData(' ');
    if (i>=14){
      sendData('A');
      sendData(i-14);
    } 
    else { 
      sendData(StringUtils::getFixedDigitsString(i, 2));
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


void WebServerClass::printSendFullLog(boolean printEvents, boolean printErrors, boolean printTemperature){
  LogRecord logRecord;
  boolean isEmpty = true;
  sendTag(FS(S_table), HTTP_TAG_OPEN);
  for (int i = 0; i < GB_Logger.getLogRecordsCount(); i++){

    logRecord = GB_Logger.getLogRecordByIndex(i);
    if (!printEvents && GB_Logger.isEvent(logRecord)){
      continue;
    }
    if (!printErrors && GB_Logger.isError(logRecord)){
      continue;
    }
    if (!printTemperature && GB_Logger.isTemperature(logRecord)){
      continue;
    }

    sendTag(FS(S_tr), HTTP_TAG_OPEN);
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendData(i+1);
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendData(StringUtils::timeToString(logRecord.timeStamp));    
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendData(StringUtils::byteToHexString(logRecord.data, true));
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendData(GB_Logger.getLogRecordDescription(logRecord));
    sendData(GB_Logger.getLogRecordSuffix(logRecord));
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
    //sendDataLn();
    sendTag(FS(S_tr), HTTP_TAG_CLOSED);
    isEmpty = false;

    if (c_isWifiResponseError) return;

  }
  sendTag(FS(S_table), HTTP_TAG_CLOSED);
  if (isEmpty){
    sendData(F("Log empty"));
  }
}

// TODO garbage?
void WebServerClass::printStorage(word address, byte sizeOf){
  byte buffer[sizeOf];
  AT24C32_EEPROM.read(address, buffer, sizeOf);
  PrintUtils::printRAM(buffer, sizeOf);
  Serial.println();
}

void WebServerClass::sendStorageDump(){
  sendTag(FS(S_table), HTTP_TAG_OPEN);
  sendTag(FS(S_tr), HTTP_TAG_OPEN);
  sendTag(FS(S_td), HTTP_TAG_OPEN);
  sendTag(FS(S_td), HTTP_TAG_CLOSED);
  for (word i = 0; i < 16 ; i++){
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendTag('b', HTTP_TAG_OPEN);
    sendData(StringUtils::byteToHexString(i));
    sendTag('b', HTTP_TAG_CLOSED); 
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
  }
  sendTag(FS(S_tr), HTTP_TAG_CLOSED);

  for (word i = 0; i < AT24C32_EEPROM.CAPACITY ; i++){
    byte value = AT24C32_EEPROM.read(i);

    if (i% 16 ==0){
      if (i>0){
        sendTag(FS(S_tr), HTTP_TAG_CLOSED);
      }
      sendTag(FS(S_tr), HTTP_TAG_OPEN);
      sendTag(FS(S_td), HTTP_TAG_OPEN);
      sendTag('b', HTTP_TAG_OPEN);
      sendData(StringUtils::byteToHexString(i/16));
      sendTag('b', HTTP_TAG_CLOSED);
      sendTag(FS(S_td), HTTP_TAG_CLOSED);
    }
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendData(StringUtils::byteToHexString(value));
    sendTag(FS(S_td), HTTP_TAG_CLOSED);

    if (c_isWifiResponseError) return;

  }
  sendTag(FS(S_tr), HTTP_TAG_CLOSED);
  sendTag(FS(S_table), HTTP_TAG_CLOSED);
}



WebServerClass GB_WebServer;





