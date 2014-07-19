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

  String url;   
  String postParams; 

  // HTTP response supplemental   
  RAK410_XBeeWifiClass::RequestType commandType = RAK410_XBeeWifi.handleSerialEvent(c_wifiPortDescriptor, url, postParams);

  switch(commandType){
    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET:
    if (
    StringUtils::flashStringEquals(url, FS(S_url_root)) || 
      StringUtils::flashStringEquals(url, FS(S_url_log)) ||
      StringUtils::flashStringStartsWith(url,FS(S_url_configuration)) ||
      StringUtils::flashStringEquals(url, FS(S_url_storage))
      ){

      sendHttpPageHeader();
      sendHttpPageBody(url);
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
    sendHttpRedirect(applyPostParams(postParams));
    break;

  default:
    break;
  }
}

/////////////////////////////////////////////////////////////////////
//                               HTTP                              //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendHttpNotFound(){ 
  RAK410_XBeeWifi.sendFixedSizeData(c_wifiPortDescriptor, F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
  RAK410_XBeeWifi.sendCloseConnection(c_wifiPortDescriptor);
}

// WARNING! RAK 410 became mad when 2 parallel connections comes. Like with Chrome and POST request, when RAK response 303.
// Connection for POST request closed by Chrome (not by RAK). And during this time Chrome creates new parallel connection for GET
// request.
void WebServerClass::sendHttpRedirect(const String &url){ 
  //const __FlashStringHelper* header = F("HTTP/1.1 303 See Other\r\nLocation: "); // DO not use it with RAK 410
  const __FlashStringHelper* header = F("HTTP/1.1 200 OK (303 doesn't work on RAK 410)\r\nrefresh: 1; url="); 

  RAK410_XBeeWifi.sendFixedSizeFrameStart(c_wifiPortDescriptor, StringUtils::flashStringLength(header) + url.length() + StringUtils::flashStringLength(FS(S_CRLFCRLF)));

  RAK410_XBeeWifi.sendFixedSizeFrameData(header);
  RAK410_XBeeWifi.sendFixedSizeFrameData(url);
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

void WebServerClass::sendHttpPageBody(const String &url){

  boolean isRootPage    = StringUtils::flashStringEquals(url, FS(S_url_root));
  boolean isLogPage     = StringUtils::flashStringEquals(url, FS(S_url_log));
  boolean isConfPage    = StringUtils::flashStringStartsWith(url, FS(S_url_configuration));
  boolean isStoragePage = StringUtils::flashStringEquals(url, FS(S_url_storage));

  sendRawData(F("<html><head>"));
  sendRawData(F("  <title>Growbox</title>"));
  sendRawData(F("  <meta name='viewport' content='width=device-width, initial-scale=1'/>"));
  sendRawData(F("</head>"));
  sendRawData(F("<body style='font-family:Arial;'>"));

  sendRawData(F("<h1>Growbox</h1>"));   

  sendTagButton(FS(S_url_root), F("Status"), isRootPage);
  sendTagButton(FS(S_url_log), F("Daily log"), isLogPage);
  sendTagButton(FS(S_url_configuration), F("Configuration"), isConfPage);
  sendTagButton(FS(S_url_storage), F("Storage dump"), isStoragePage);
  sendTag(FS(S_hr), HTTP_TAG_SINGLE);

  //sendTag(FS(S_pre), HTTP_TAG_OPEN);
  if (isRootPage){
    sendStatusPage();
  } 
  else if (isLogPage){
    sendFullLog(true, true, true); // TODO use parameters
  }
  else if (isConfPage){
    sendConfigurationForms();
  } 
  else if (isStoragePage){
    sendStorageDump(); 
  }

  if (c_isWifiResponseError) return;

  sendRawData(F("</body></html>"));
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
//                               HTML                              //
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

void WebServerClass::sendTagButton(const __FlashStringHelper* buttonUrl, const __FlashStringHelper* buttonTitle, boolean isSelected){
  sendRawData(F("<input type='button' onclick='document.location=\""));
  sendRawData(buttonUrl);
  sendRawData(F("\"' value='"));
  sendRawData(buttonTitle);
  sendRawData(F("'"));
  if (isSelected) {
    sendRawData(F(" style='text-decoration:underline;'"));
  }
  sendRawData(F(" />"));
}

/////////////////////////////////////////////////////////////////////
//                          STATUS PAGE                            //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendStatusPage(){
  sendRawData(F("<dl>"));

  sendRawData(F("<dt>Common</dt>"));
  
  sendRawData(F("<dd>")); 
  sendRawData(g_isDayInGrowbox ? F("day") : F("night"));
  sendRawData(F("mode</dd>"));
  
  sendRawData(F("<dd>Current time: ")); 
  sendRawData(now());
  sendRawData(F("</dd>")); 
   
  sendRawData(F("<dd>Last startup: ")); 
  sendRawData(GB_StorageHelper.getLastStartupTimeStamp());
  sendRawData(F("</dd>")); 

  sendRawData(F("<dt>Scheduler</dt>"));
  
  sendRawData(F("<dd>Day mode up time: ")); 
  sendRawData(UP_HOUR); 
  sendRawData(F(":00</dd><dd>Night mode down time:"));
  sendRawData(DOWN_HOUR);
  sendRawData(F(":00</dd>"));


  sendRawData(F("<dt>Logger</dt>"));
  
  sendRawData(F("<dd>"));  
  sendRawData(GB_StorageHelper.isStoreLogRecordsEnabled() ? F("Enabled") : F("<b>Disabled</b>"));
  sendRawData(F("</dd>")); 
  
  sendRawData(F("<dd>Stored records: "));
  sendRawData(GB_StorageHelper.getLogRecordsCount());
  sendRawData('/');
  sendRawData(GB_StorageHelper.LOG_CAPACITY);
  if (GB_StorageHelper.isLogOverflow()){
    sendRawData(F(", <b>overflow</b>"));
  }
  sendRawData(F("</dd>"));

  
  sendRawData(F("<dt>Other</dt>"));
  
  sendRawData(F("<dd>Free memory: "));
  sendRawData(freeMemory()); 
  sendRawData(F(" bytes</dd>"));

  sendRawData(F("<dd>First startup: ")); 
  sendRawData(GB_StorageHelper.getFirstStartupTimeStamp());
  sendRawData(F("</dd>")); 




  float workingTemperature, statisticsTemperature;
  int statisticsCount;
  GB_Thermometer.getStatistics(workingTemperature, statisticsTemperature, statisticsCount);

  sendRawData(F("<dd>Temperature: ")); 
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



 // sendBootStatus();
//  sendTimeStatus();
//  sendTemperatureStatus();  
  sendRawData(F("</dl>"));
  sendTag(FS(S_hr), HTTP_TAG_SINGLE); 
  sendPinsStatus();   
}

void WebServerClass::sendFreeMemory(){ 
  sendRawData(F("<dt>Hardware</dt><dd>Free memory: "));
  sendRawData(freeMemory()); 
  sendRawData(F(" bytes</dd>"));
}

void WebServerClass::sendBootStatus(){
  sendRawData(F("<dt>Controller</dt><dd> startup: ")); 
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
  sendRawData(F("</dd>"));
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

/////////////////////////////////////////////////////////////////////
//                      CONFIGURATION PAGE                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendConfigurationForms(){
  sendWiFIConfigurationForm();
}

void WebServerClass::sendWiFIConfigurationForm(){

  boolean isWifiStationMode = GB_StorageHelper.isWifiStationMode();

  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post'>"));

  sendRawData(F("<fieldset><legend>Wi-Fi</legend>"));

  sendRawData(F("<input type='radio' name='isWifiStationMode' value='0'")); 
  if(!isWifiStationMode) {
    sendRawData(F("checked='checked'")); 
  } 
  sendRawData(F("/>Create new Network<br/>Hidden, security WPA2, ip 192.168.0.1<br/>"));
  sendRawData(F("<input type='radio' name='isWifiStationMode' value='1'"));
  if(isWifiStationMode) {
    sendRawData(F("checked='checked'")); 
  } 
  sendRawData(F("/>Join existed Network<br/>"));

  sendRawData(F("<table>"));
  sendRawData(F("<tr><td><label for='wifiSSID'>SSID</label></td><td><input type='text' name='wifiSSID' required value='"));
  sendRawData(GB_StorageHelper.getWifiSSID());
  sendRawData(F("' maxlength='"));
  sendRawData(WIFI_SSID_LENGTH);
  sendRawData(F("'/></td></tr>"));

  sendRawData(F("<tr><td><label for='wifiPass'>Password</label></td><td><input type='text' name='wifiPass' required value='"));
  sendRawData(GB_StorageHelper.getWifiPass());
  sendRawData(F("' maxlength='"));
  sendRawData(WIFI_PASS_LENGTH);
  sendRawData(F("'/></td></tr>"));

  sendRawData(F("</table>")); 

  sendRawData(F("<input type='submit' value='Save'> and reboot device manually"));

  sendRawData(F("</fieldset>"));
  sendRawData(F("</form>"));
}

String WebServerClass::applyPostParams(String& postParams){
  boolean isAllParamsApplied = true;
  int beginIndex = 0;  
  int endIndex = postParams.indexOf('&');
  while (beginIndex < (int) postParams.length()){
    if (endIndex == -1){
      endIndex = postParams.length();
    }
    String postParam = postParams.substring(beginIndex, endIndex);  
    isAllParamsApplied &= applyPostParam(postParam);

    beginIndex = endIndex+1;
    endIndex = postParams.indexOf('&', beginIndex);
  }

  String rez = StringUtils::flashStringLoad(FS(S_url_configuration));
  if (isAllParamsApplied){
    rez += StringUtils::flashStringLoad(F("/ok"));
  } 
  else {
    rez += StringUtils::flashStringLoad(F("/error"));
  }
  return rez;
}

boolean WebServerClass::applyPostParam(String& postParam){
  // isWifiStationMode=1&wifiSSID=Growbox%3C&wifiPass=ingodwetrust
  boolean isOK = true;
  //Serial.println(postParam);
  int equalsCharIndex = postParam.indexOf('=');
  if (equalsCharIndex == -1){
    return false;
  }
  String paramName  = postParam.substring(0, equalsCharIndex);
  String paramValue = postParam.substring(equalsCharIndex+1);

  //Serial.print(paramName); Serial.print(" -> "); Serial.println(paramValue);

  if (StringUtils::flashStringEquals(paramName, F("isWifiStationMode"))){
    GB_StorageHelper.setWifiStationMode(paramValue[0]=='1');  
  } 
  else if (StringUtils::flashStringEquals(paramName, F("wifiSSID"))){
    GB_StorageHelper.setWifiSSID(paramValue);
  }
  else if (StringUtils::flashStringEquals(paramName, F("wifiPass"))){
    GB_StorageHelper.setWifiPass(paramValue);
  }

  return isOK;
}

/////////////////////////////////////////////////////////////////////
//                          OTHER PAGES                            //
/////////////////////////////////////////////////////////////////////

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























