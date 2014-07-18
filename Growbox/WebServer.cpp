#include "WebServer.h"

#include "RAK410_XBeeWifi.h" 
#include "EEPROM_AT24C32.h" 

void WebServerClass::showWebMessage(const __FlashStringHelper* str, boolean newLine){ //TODO 
  if (g_useSerialMonitor){
    Serial.print(F("WEB> "));
    Serial.print(str);
    if (newLine){  
      Serial.println();    
    }      
  }
}

/////////////////////////////////////////////////////////////////////
//                           HTTP PROTOCOL                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::handleSerialEvent(){

  String input;   
  String postParams; 

  // HTTP response supplemental   
  RAK410_XBeeWifiClass::RequestType commandType = RAK410_XBeeWifi.handleSerialEvent(c_wifiPortDescriptor, input, postParams);

  switch(commandType){
    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET:
    if (
      StringUtils::flashStringEquals(input, FS(S_url)) || 
      StringUtils::flashStringEquals(input, FS(S_url_log)) ||
      StringUtils::flashStringEquals(input, FS(S_url_conf)) ||
      StringUtils::flashStringEquals(input, FS(S_url_storage))
    ){

      sendHttpPageHeader();
      sendHttpPageBody(input);
      sendHttpPageComplete();

      if(g_useSerialMonitor){ 
        if (c_isWifiResponseError){
          showWebMessage(F("Error occurred during sending responce"));
        }
      }
    } 
    else { // Unknown resource
      sendHttpNotFound();    
    }
    break;  

    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST:
    sendHttpRedirect(FS(S_url));
    break;

  default:
    break;
  }
}


/////////////////////////////////////////////////////////////////////
//                           HTTP PROTOCOL                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendHttpNotFound(){ 
  RAK410_XBeeWifi.sendFixedSizeData(c_wifiPortDescriptor, F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
  RAK410_XBeeWifi.sendCloseConnection(c_wifiPortDescriptor);
}

// WARNING! RAK 410 became mad when 2 parallel connections comes. Like with Chrome and POST request, when RAK response 303.
// Connection for POST request closed by Chrome (not by RAK). And during this time Chrome creates new parallel connection for GET
// request.
void WebServerClass::sendHttpRedirect(const __FlashStringHelper* data){ 
  //const __FlashStringHelper* header = F("HTTP/1.1 303 See Other\r\nLocation: "); // DO not use it with RAK 410
  const __FlashStringHelper* header = F("HTTP/1.1 200 OK (303 doesn't work on RAK 410)\r\nrefresh: 0; url="); 

  RAK410_XBeeWifi.sendFixedSizeFrameStart(c_wifiPortDescriptor, StringUtils::flashStringLength(header) + StringUtils::flashStringLength(data) + StringUtils::flashStringLength(FS(S_CRLFCRLF)));

  RAK410_XBeeWifi.sendFixedSizeFrameData(header);
  RAK410_XBeeWifi.sendFixedSizeFrameData(data);
  RAK410_XBeeWifi.sendFixedSizeFrameData(FS(S_CRLFCRLF));

  RAK410_XBeeWifi.sendFixedSizeFrameStop();

  RAK410_XBeeWifi.sendCloseConnection(c_wifiPortDescriptor);
}


void WebServerClass::sendHttpPageHeader(){ 
  RAK410_XBeeWifi.sendFixedSizeData(c_wifiPortDescriptor, F("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"));
  RAK410_XBeeWifi.sendAutoSizeFrameStart(c_wifiPortDescriptor);
}

void WebServerClass::sendHttpPageComplete(){  
  RAK410_XBeeWifi.sendAutoSizeFrameStop();
  RAK410_XBeeWifi.sendCloseConnection(c_wifiPortDescriptor);
}

void WebServerClass::sendHttpPageBody(const String &input){

  sendTag(FS(S_html), HTTP_TAG_OPEN); 
  sendTag(FS(S_h1), HTTP_TAG_OPEN); 
  sendRawData(F("Growbox"));  
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
    sendFullLog(true, true, true); // TODO use parameters
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


/////////////////////////////////////////////////////////////////////
//                    HTML SUPPLEMENTAL COMMANDS                   //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendRawData(const __FlashStringHelper* data){
  if (!RAK410_XBeeWifi.sendAutoSizeFrameData(c_wifiPortDescriptor, data)){
    c_isWifiResponseError = true;
  }
} 

void WebServerClass::sendRawData(const String &data){
  if (!RAK410_XBeeWifi.sendAutoSizeFrameData(c_wifiPortDescriptor, data)){
    c_isWifiResponseError = true;
  }
}

void WebServerClass::sendRawData(float data){
  String str = StringUtils::floatToString(data);
  sendRawData(str);
}

void WebServerClass::sendRawData(time_t data){
  String str = StringUtils::timeToString(data);
  sendRawData(str);
}

void WebServerClass::sendTagButton(const __FlashStringHelper* url, const __FlashStringHelper* name){
  sendRawData(F("<input type=\"button\" onclick=\"document.location='"));
  sendRawData(url);
  sendRawData(F("'\" value=\""));
  sendRawData(name);
  sendRawData(F("\"/>"));
}

/////////////////////////////////////////////////////////////////////
//                          HTML PAGE PARTS                        //
/////////////////////////////////////////////////////////////////////


void WebServerClass::sendBriefStatus(){
  sendFreeMemory();
  sendBootStatus();
  sendTimeStatus();
  sendTemperatureStatus();  
  sendTag(FS(S_hr), HTTP_TAG_SINGLE); 
}

void WebServerClass::sendConfigurationControls(){
  sendRawData(F("<form action=\"/\" method=\"post\">"));
  sendRawData(F("<input type=\"submit\" value=\"Submit\">"));
  sendRawData(F("</form>"));
}

void WebServerClass::sendFreeMemory(){  
  sendRawData(FS(S_Free_memory));
  //sendTab_B(HTTP_TAG_OPEN);
  sendRawData(freeMemory()); 
  sendRawData(FS(S_bytes));
  sendRawData(FS(S_CRLF));
}

void WebServerClass::sendBootStatus(){
  sendRawData(F("Controller: startup: ")); 
  sendRawData(GB_StorageHelper.getLastStartupTimeStamp());
  sendRawData(F(", first startup: ")); 
  sendRawData(GB_StorageHelper.getFirstStartupTimeStamp());
  sendRawData(F("\r\nLogger: "));
  if (GB_StorageHelper.isStoreLogRecordsEnabled()){
    sendRawData(FS(S_Enabled));
  } 
  else {
    sendRawData(FS(S_Disabled));
  }
  sendRawData(F(", records "));
  sendRawData(GB_StorageHelper.getLogRecordsCount());
  sendRawData('/');
  sendRawData(GB_StorageHelper.LOG_CAPACITY);
  if (GB_StorageHelper.isLogOverflow()){
    sendRawData(F(", overflow"));
  } 
  sendRawData(FS(S_CRLF));
}

void WebServerClass::sendTimeStatus(){
  sendRawData(F("Clock: ")); 
  sendTag('b', HTTP_TAG_OPEN);
  if (g_isDayInGrowbox) {
    sendRawData(F("DAY"));
  } 
  else{
    sendRawData(F("NIGHT"));
  }
  sendTag('b', HTTP_TAG_CLOSED);
  sendRawData(F(" mode, time ")); 
  sendRawData(now());
  sendRawData(F(", up time [")); 
  sendRawData(UP_HOUR); 
  sendRawData(F(":00], down time ["));
  sendRawData(DOWN_HOUR);
  sendRawData(F(":00]\r\n"));
}

void WebServerClass::sendTemperatureStatus(){
  float workingTemperature, statisticsTemperature;
  int statisticsCount;
  GB_Thermometer.getStatistics(workingTemperature, statisticsTemperature, statisticsCount);

  sendRawData(FS(S_Temperature)); 
  sendRawData(F(": current ")); 
  sendRawData(workingTemperature);
  sendRawData(F(", next ")); 
  sendRawData(statisticsTemperature);
  sendRawData(F(" (count ")); 
  sendRawData(statisticsCount);

  sendRawData(F("), day "));
  sendRawData(TEMPERATURE_DAY);
  sendRawData(FS(S_PlusMinus));
  sendRawData(TEMPERATURE_DELTA);
  sendRawData(F(", night "));
  sendRawData(TEMPERATURE_NIGHT);
  sendRawData(FS(S_PlusMinus));
  sendRawData(2*TEMPERATURE_DELTA);
  sendRawData(F(", critical "));
  sendRawData(TEMPERATURE_CRITICAL);
  sendRawData(FS(S_CRLF));
}

void WebServerClass::sendPinsStatus(){
  sendRawData(F("Pin OUTPUT INPUT")); 
  sendRawData(FS(S_CRLF));
  for(int i=0; i<=19;i++){
    sendRawData(' ');
    if (i>=14){
      sendRawData('A');
      sendRawData(i-14);
    } 
    else { 
      sendRawData(StringUtils::getFixedDigitsString(i, 2));
    }
    sendRawData(FS(S___)); 

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
      sendRawData(FS(S___));
      sendRawData(dataStatus);
      sendRawData(F("     -   "));
    } 
    else {
      sendRawData(F("  -     "));
      sendRawData(inputStatus);
      sendRawData(FS(S___));
    }

    switch(i){
    case 0: 
    case 1: 
      sendRawData(F("Reserved by Serial/USB. Can be used, if Serial/USB won't be connected"));
      break;
    case LIGHT_PIN: 
      sendRawData(F("Relay: light on(0)/off(1)"));
      break;
    case FAN_PIN: 
      sendRawData(F("Relay: fan on(0)/off(1)"));
      break;
    case FAN_SPEED_PIN: 
      sendRawData(F("Relay: fun max(0)/min(1) speed switch"));
      break;
    case ONE_WIRE_PIN: 
      sendRawData(F("1-Wire: termometer"));
      break;
    case USE_SERIAL_MONOTOR_PIN: 
      sendRawData(F("Use serial monitor on(1)/off(0)"));
      break;
    case ERROR_PIN: 
      sendRawData(F("Error status"));
      break;
    case BREEZE_PIN: 
      sendRawData(F("Breeze"));
      break;
    case 18: 
    case 19: 
      sendRawData(F("Reserved by I2C. Can be used, if SCL, SDA pins will be used"));
      break;
    }
    sendRawData(FS(S_CRLF));
  }
}


void WebServerClass::sendFullLog(boolean printEvents, boolean printErrors, boolean printTemperature){
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
    sendRawData(i+1);
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendRawData(StringUtils::timeToString(logRecord.timeStamp));    
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendRawData(StringUtils::byteToHexString(logRecord.data, true));
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendRawData(GB_Logger.getLogRecordDescription(logRecord));
    sendRawData(GB_Logger.getLogRecordSuffix(logRecord));
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
    //sendDataLn();
    sendTag(FS(S_tr), HTTP_TAG_CLOSED);
    isEmpty = false;

    if (c_isWifiResponseError) return;

  }
  sendTag(FS(S_table), HTTP_TAG_CLOSED);
  if (isEmpty){
    sendRawData(F("Log empty"));
  }
}

// TODO garbage?
void WebServerClass::printStorage(word address, byte sizeOf){
  byte buffer[sizeOf];
  EEPROM_AT24C32.readBlock<byte>(address, buffer, sizeOf);
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
    sendRawData(StringUtils::byteToHexString(i));
    sendTag('b', HTTP_TAG_CLOSED); 
    sendTag(FS(S_td), HTTP_TAG_CLOSED);
  }
  sendTag(FS(S_tr), HTTP_TAG_CLOSED);

  for (word i = 0; i < EEPROM_AT24C32.getCapacity() ; i++){
    byte value = EEPROM_AT24C32.read(i);

    if (i% 16 ==0){
      if (i>0){
        sendTag(FS(S_tr), HTTP_TAG_CLOSED);
      }
      sendTag(FS(S_tr), HTTP_TAG_OPEN);
      sendTag(FS(S_td), HTTP_TAG_OPEN);
      sendTag('b', HTTP_TAG_OPEN);
      sendRawData(StringUtils::byteToHexString(i/16));
      sendTag('b', HTTP_TAG_CLOSED);
      sendTag(FS(S_td), HTTP_TAG_CLOSED);
    }
    sendTag(FS(S_td), HTTP_TAG_OPEN);
    sendRawData(StringUtils::byteToHexString(value));
    sendTag(FS(S_td), HTTP_TAG_CLOSED);

    if (c_isWifiResponseError) return;

  }
  sendTag(FS(S_tr), HTTP_TAG_CLOSED);
  sendTag(FS(S_table), HTTP_TAG_CLOSED);
}



WebServerClass GB_WebServer;














