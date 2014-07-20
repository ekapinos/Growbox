#include "WebServer.h"

#include <MemoryFree.h>
#include "StorageHelper.h" 
#include "Thermometer.h" 
#include "RAK410_XBeeWifi.h" 
#include "EEPROM_AT24C32.h" 

/////////////////////////////////////////////////////////////////////
//                           HTTP PROTOCOL                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::handleSerialEvent(){

  String url, getParams, postParams; 

  // HTTP response supplemental   
  RAK410_XBeeWifiClass::RequestType commandType = RAK410_XBeeWifi.handleSerialEvent(c_wifiPortDescriptor, url, getParams, postParams);

  switch(commandType){
    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET:
    if (
    StringUtils::flashStringEquals(url, FS(S_url_root)) || 
      //StringUtils::flashStringEquals(url, FS(S_url_pins)) || 
    StringUtils::flashStringEquals(url, FS(S_url_log)) ||
      StringUtils::flashStringStartsWith(url,FS(S_url_configuration)) ||
      StringUtils::flashStringEquals(url, FS(S_url_storage))
      ){

      sendHttpPageHeader();
      sendHttpPageBody(url, getParams);
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

void WebServerClass::sendHttpPageBody(const String& url, const String& getParams){

  boolean isRootPage    = StringUtils::flashStringEquals(url, FS(S_url_root));
  //  boolean isPinsPage    = StringUtils::flashStringEquals(url, FS(S_url_pins));
  boolean isLogPage     = StringUtils::flashStringEquals(url, FS(S_url_log));
  boolean isConfPage    = StringUtils::flashStringStartsWith(url, FS(S_url_configuration));
  boolean isStoragePage = StringUtils::flashStringEquals(url, FS(S_url_storage));

  sendRawData(F("<html><head>"));
  sendRawData(F("  <title>Growbox</title>"));
  sendRawData(F("  <meta name='viewport' content='width=device-width, initial-scale=1'/>"));
  if (isLogPage){
    sendLogPageStyles();
  }
  sendRawData(F("</head>"));
  //<STYLE type="text/css">
  //  #myid {border-width: 1; border: solid; text-align: center}
  //</STYLE>
  sendRawData(F("<body style='font-family:Arial;'>"));

  sendRawData(F("<h1>Growbox</h1>"));   

  sendTagButton(FS(S_url_root), F("Status"), isRootPage);
  //sendTagButton(FS(S_url_pins), F("Pins status"), isPinsPage);  
  sendTagButton(FS(S_url_log), F("Daily log"), isLogPage);
  sendTagButton(FS(S_url_configuration), F("Configuration"), isConfPage);
  sendTagButton(FS(S_url_storage), F("Storage dump"), isStoragePage);

  sendRawData(F("<hr/>"));

  if (isRootPage){
    sendStatusPage();
  } 
  //  if (isPinsPage){
  //    sendPinsStatus();
  //  } 
  else if (isLogPage){
    sendLogPage(String(), true, true, true); // TODO use parameters
  }
  else if (isConfPage){
    sendConfigurationPage(getParams);
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

String WebServerClass::encodeHttpString(const String& dirtyValue){

  String value;
  for(unsigned int i = 0; i< dirtyValue.length(); i++){
    if (dirtyValue[i] == '+'){
      value += ' '; 
    } 
    else if (dirtyValue[i] == '%'){
      if (i + 2 < dirtyValue.length()){
        byte hiCharPart = StringUtils::hexCharToByte(dirtyValue[i+1]);
        byte loCharPart = StringUtils::hexCharToByte(dirtyValue[i+2]);
        if (hiCharPart != 0xFF && loCharPart != 0xFF){  
          value += (char)((hiCharPart<<4) + loCharPart);   
        }
      }
      i += 2;
    } 
    else {
      value += dirtyValue[i];
    }
  }
  return value;
}

boolean WebServerClass::getHttpParamByIndex(const String& params, const word index, String& name, String& value){

  word paramsCount = 0;

  int beginIndex = 0;
  int endIndex = params.indexOf('&');
  while (beginIndex < (int) params.length()){
    if (endIndex == -1){
      endIndex = params.length();
    }

    if (paramsCount == index){
      String param = params.substring(beginIndex, endIndex);  

      int equalsCharIndex = param.indexOf('=');
      if (equalsCharIndex == -1){
        name = encodeHttpString(param);
        value = String();
      } 
      else {
        name  = encodeHttpString(param.substring(0, equalsCharIndex));
        value = encodeHttpString(param.substring(equalsCharIndex+1));
      }
      return true;
    }

    paramsCount++;

    beginIndex = endIndex+1;
    endIndex = params.indexOf('&', beginIndex);
  } 

  return false;
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
  String str = StringUtils::timeStampToString(data);
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
  sendRawData(g_isDayInGrowbox ? F("Day") : F("Night"));
  sendRawData(F(" mode</dd>"));

  sendRawData(F("<dd>Light: ")); 
  sendRawData((digitalRead(LIGHT_PIN) == RELAY_ON) ? F("enabled") : F("disabled"));
  sendRawData(F("</dd>"));

  sendRawData(F("<dd>Fan: ")); 
  boolean isFanEnabled = (digitalRead(FAN_PIN) == RELAY_ON);
  sendRawData(isFanEnabled ? F("enabled, ") : F("disabled"));
  if (isFanEnabled){
    sendRawData((digitalRead(FAN_SPEED_PIN) == FAN_SPEED_MIN) ? F("min") : F("max"));
  }
  sendRawData(F(" speed</dd>"));

  sendRawData(F("<dd>Current time: ")); 
  sendRawData(now());
  sendRawData(F("</dd>")); 

  sendRawData(F("<dd>Last startup: ")); 
  sendRawData(GB_StorageHelper.getLastStartupTimeStamp());
  sendRawData(F("</dd>")); 


  float workingTemperature, statisticsTemperature;
  int statisticsCount;
  GB_Thermometer.getStatistics(workingTemperature, statisticsTemperature, statisticsCount);

  sendRawData(F("<dt>Temperature</dt>")); 
  sendRawData(F("<dd>Current: ")); 
  sendRawData(workingTemperature);
  sendRawData(F(" C</dd><dd>Forecast: ")); 
  sendRawData(statisticsTemperature);
  sendRawData(F(" C (")); 
  sendRawData(statisticsCount);
  sendRawData(F(" measurements)</dd>"));


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


  // TODO remove it
  sendRawData(F("<dt>Configuration</dt>"));  

  sendRawData(F("<dd>Turn to Day mode at: ")); 
  sendRawData(UP_HOUR); 
  sendRawData(F(":00</dd><dd>Turn to Night mode at: "));
  sendRawData(DOWN_HOUR);
  sendRawData(F(":00</dd>"));

  sendRawData(F("<dd>Day temperature: "));
  sendRawData(TEMPERATURE_DAY);
  sendRawData(FS(S_PlusMinus));
  sendRawData(TEMPERATURE_DELTA);
  sendRawData(F("</dd><dd>Night temperature: "));
  sendRawData(TEMPERATURE_NIGHT);
  sendRawData(FS(S_PlusMinus));
  sendRawData(2*TEMPERATURE_DELTA);
  sendRawData(F("</dd><dd>Critical temperature: "));
  sendRawData(TEMPERATURE_CRITICAL);
  sendRawData(F("</dd>"));


  sendRawData(F("<dt>Other</dt>"));

  sendRawData(F("<dd>Free memory: "));
  sendRawData(freeMemory()); 
  sendRawData(F(" bytes</dd>"));

  sendRawData(F("<dd>First startup: ")); 
  sendRawData(GB_StorageHelper.getFirstStartupTimeStamp());
  sendRawData(F("</dd>")); 


  sendRawData(F("</dl>")); 
}

/////////////////////////////////////////////////////////////////////
//                      CONFIGURATION PAGE                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendConfigurationPage(const String& getParams){

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

String WebServerClass::applyPostParams(const String& postParams){

  String queryStr;

  word index = 0;  
  String name, value;
  while(getHttpParamByIndex(postParams, index, name, value)){

    if (queryStr.length() > 0){
      queryStr += '&';
    }
    queryStr += name;
    queryStr += '=';   
    queryStr += applyPostParam(name, value);

    index++;
  }

  if (index > 0){
    return StringUtils::flashStringLoad(FS(S_url_configuration)) + String('?') + queryStr;
  }
  else {
    return StringUtils::flashStringLoad(FS(S_url_configuration));
  }
}

boolean WebServerClass::applyPostParam(const String& name, const String& value){

  if (StringUtils::flashStringEquals(name, F("isWifiStationMode"))){
    if (value.length() != 1){
      return false;
    }
    GB_StorageHelper.setWifiStationMode(value[0]=='1'); 

  } 
  else if (StringUtils::flashStringEquals(name, F("wifiSSID"))){
    GB_StorageHelper.setWifiSSID(value);

  }
  else if (StringUtils::flashStringEquals(name, F("wifiPass"))){
    GB_StorageHelper.setWifiPass(value);

  } 
  else {
    return false;
  }

  return true;
}

/////////////////////////////////////////////////////////////////////
//                          OTHER PAGES                            //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendLogPageStyles(){
  sendRawData(F("  <style type='text/css'>"));
  sendRawData(F("    table.log {border-spacing: 5px;}"));
  sendRawData(F("    table.log td.ref {text-align: center;}")); 
  sendRawData(F("    table.log td.current {text-align: center; font-weight: bold;}"));
  sendRawData(F("    table.log th {text-align: center; font-weight: bold;}"));  
  sendRawData(F("  </style>"));
}

void WebServerClass::sendLogPage(String getParams, boolean printEvents, boolean printErrors, boolean printTemperature){

  LogRecord logRecord;

  tmElements_t previousTm;
  tmElements_t currentTm;
  tmElements_t targetTm;

  previousTm.Day = previousTm.Month = previousTm.Year = 0; //remove compiller warning
  breakTime(now(), targetTm);


  sendRawData(F("<table class='log'>"));
  word index;
  for (index = 0; index < GB_Logger.getLogRecordsCount(); index++){

    logRecord = GB_Logger.getLogRecordByIndex(index);
    if (!printEvents && GB_Logger.isEvent(logRecord)){
      continue;
    } 
    else if (!printErrors && GB_Logger.isError(logRecord)){
      continue;
    } 
    else if (!printTemperature && GB_Logger.isTemperature(logRecord)){
      continue;
    }

    breakTime(logRecord.timeStamp, currentTm);

    boolean isDaySwitch = !(currentTm.Day == previousTm.Day && currentTm.Month == previousTm.Month && currentTm.Year == previousTm.Year);
    if (isDaySwitch){
      previousTm = currentTm; 
    }

    boolean isTargetDay = (currentTm.Day == targetTm.Day && currentTm.Month == targetTm.Month && currentTm.Year == targetTm.Year);

    if (isDaySwitch){ 

      if (isTargetDay){
        sendRawData(F("<tr><td class='current' colspan='3'> "));
        sendRawData(StringUtils::timeStampToString(logRecord.timeStamp, true, false));
        sendRawData(F("</td></tr>")); 
        sendRawData(F("<tr><th>#</th><th>Time</th><th>Description</th></tr>"));
      } 
      else {
        sendRawData(F("<tr><td class='ref' colspan='3'> "));
        sendRawData(F("<a href='"));
        sendRawData(FS(S_url_log));
        sendRawData(F("&date="));
        String dateInUrl = StringUtils::timeStampToString(logRecord.timeStamp, true, false);
        dateInUrl.replace(String('.'), String());
        sendRawData(dateInUrl);
        sendRawData(F("'>"));
        sendRawData(StringUtils::timeStampToString(logRecord.timeStamp, true, false));
        sendRawData(F("</a>"));
        sendRawData(F("</td></tr> "));
      } 

    }

    if (!isTargetDay){
      continue;
    } 

    sendRawData(F("<tr><td>"));
    sendRawData(index+1);
    sendRawData(F("</td><td>"));
    sendRawData(StringUtils::timeStampToString(logRecord.timeStamp, false, true));  //print Time  
    //sendRawData(F("</td><td>"));
    //sendRawData(StringUtils::byteToHexString(logRecord.data, true));
    sendRawData(F("</td><td>"));
    sendRawData(GB_Logger.getLogRecordDescription(logRecord));
    sendRawData(GB_Logger.getLogRecordDescriptionSuffix(logRecord));
    sendRawData(F("</td></tr>")); // bug with linker was here https://github.com/arduino/Arduino/issues/1071#issuecomment-19832135

    if (c_isWifiResponseError) return;

  }
  sendRawData(F("</table>"));

  if (index == 0){
    sendRawData(F("No records"));
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




































