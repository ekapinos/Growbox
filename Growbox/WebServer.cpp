#include "WebServer.h"

#include <MemoryFree.h>
#include "Controller.h"
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

  c_isWifiResponseError = false;

  switch(commandType){
    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET:
    if (StringUtils::flashStringEquals(url, FS(S_url_root)) || 
      //StringUtils::flashStringEquals(url, FS(S_url_pins)) || 
    StringUtils::flashStringEquals(url, FS(S_url_log)) ||
      StringUtils::flashStringEquals(url,FS(S_url_configuration)) ||
      StringUtils::flashStringEquals(url, FS(S_url_storage))){

      sendHttpPageHeader();
      sendHttpPageBody(url, getParams);
      sendHttpPageComplete();
    } 
    else { // Unknown resource
      sendHttpNotFound();    
    }
    break;  

    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST: 
    sendHttpRedirect(applyPostParams(url, postParams));
    break;

  default:
    break;
  }

  if(g_useSerialMonitor){ 
    if (c_isWifiResponseError){
      showWebMessage(F("Error occurred during sending responce"));
    }
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

/////////////////////////////////////////////////////////////////////
//                         HTTP PARAMETERS                         //
/////////////////////////////////////////////////////////////////////

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

boolean WebServerClass::searchHttpParamByName(const String& params, const __FlashStringHelper* targetName, String& targetValue){
  String name, value;
  word index = 0;
  while(getHttpParamByIndex(params, index, name, value)){
    if (StringUtils::flashStringEquals(name, targetName)){
      targetValue = value;
      return true;
    }
    index++;
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

void WebServerClass::sendTagButton(const __FlashStringHelper* url, const __FlashStringHelper* text, boolean isSelected){
  sendRawData(F("<input type='button' onclick='document.location=\""));
  sendRawData(url);
  sendRawData(F("\"' value='"));
  sendRawData(text);
  sendRawData(F("' "));
  if (isSelected) {
    sendRawData(F("style='text-decoration:underline;' "));
  }
  sendRawData(F("/>"));
}

void WebServerClass::sendTagRadioButton(const __FlashStringHelper* name, const __FlashStringHelper* text, const __FlashStringHelper* value, boolean isSelected){
  sendRawData(F("<label><input type='radio' name='"));
  sendRawData(name);
  sendRawData(F("' value='"));
  sendRawData(value);
  sendRawData(F("' "));
  if (isSelected){
    sendRawData(F("checked='checked' "));
  }
  sendRawData(F("/>"));
  sendRawData(text);
  sendRawData(F("</label>")); 
}

void WebServerClass::sendTagCheckbox(const __FlashStringHelper* name, const __FlashStringHelper* text, boolean isSelected){
  sendRawData(F("<label><input type='checkbox' "));
  if (isSelected){
    sendRawData(F("checked='checked' "));
  }
  sendRawData(F("onchange='document.getElementById(\""));
  sendRawData(name);
  sendRawData(F("\").value = (this.checked?1:0)'/>"));
  sendRawData(text);
  sendRawData(F("</label>"));
  sendRawData(F("<input type='hidden' name='"));
  sendRawData(name);
  sendRawData(F("' id='"));
  sendRawData(name);
  sendRawData(F("' value='"));
  sendRawData(isSelected?'1':'0');
  sendRawData(F("'>"));  

}

void WebServerClass::sendAppendOptionToSelectDynamic(const __FlashStringHelper* selectId, const String& value, const String& text, boolean isSelected){
  sendRawData(F("<script type='text/javascript'>"));
  sendRawData(F("var opt = document.createElement('option');"));
  if (value.length() != 0){
    sendRawData(F("opt.value = '"));   
    sendRawData(value);
    sendRawData(F("';"));
  }
  sendRawData(F("opt.text = '"));    //opt.value = i;
  sendRawData(text);
  sendRawData(F("';"));
  if (isSelected){
    sendRawData(F("opt.selected = true;"));
    sendRawData(F("opt.style.fontWeight = 'bold';"));
  }   
  sendRawData(F("document.getElementById('"));
  sendRawData(selectId);
  sendRawData(F("').appendChild(opt);"));
  sendRawData(F("</script>"));
}

void WebServerClass::sendAppendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const String& text, boolean isSelected){
  sendAppendOptionToSelectDynamic(selectId, StringUtils::flashStringLoad(value), text, isSelected);
}

void WebServerClass::sendAppendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const __FlashStringHelper* text, boolean isSelected){
  sendAppendOptionToSelectDynamic(selectId, StringUtils::flashStringLoad(value), StringUtils::flashStringLoad(text), isSelected);
}

/////////////////////////////////////////////////////////////////////
//                        COMMON FOR PAGES                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendHttpPageBody(const String& url, const String& getParams){

  boolean isRootPage    = StringUtils::flashStringEquals(url, FS(S_url_root));
  //  boolean isPinsPage    = StringUtils::flashStringEquals(url, FS(S_url_pins));
  boolean isLogPage     = StringUtils::flashStringEquals(url, FS(S_url_log));
  boolean isConfPage    = StringUtils::flashStringEquals(url, FS(S_url_configuration));
  boolean isStoragePage = StringUtils::flashStringEquals(url, FS(S_url_storage));

  sendRawData(F("<!DOCTYPE html>")); // HTML 5
  sendRawData(F("<html><head>"));
  sendRawData(F("  <title>Growbox</title>"));
  sendRawData(F("  <meta name='viewport' content='width=device-width, initial-scale=1'/>"));  
  sendRawData(F("<style type='text/css'>"));
  sendRawData(F("  body {font-family:Arial;}"));
  sendRawData(F("  form {margin: 0px;}"));
  sendRawData(F("  .red {color: red;}"));
  sendRawData(F("  table.grab {border-spacing:5px; width:100%; text-align:center; }")); 
  sendRawData(F("  dt {font-weight: bold; margin-top: 5px;}")); 
  sendRawData(F("</style>"));  
  sendRawData(F("</head>"));

  sendRawData(F("<body>"));
  sendRawData(F("<h1>Growbox</h1>"));   
  sendRawData(F("<form>"));   // HTML Validator warning
  sendTagButton(FS(S_url_root), F("Status"), isRootPage);
  //sendTagButton(FS(S_url_pins), F("Pins status"), isPinsPage);  
  sendTagButton(FS(S_url_log), F("Daily log"), isLogPage);
  sendTagButton(FS(S_url_configuration), F("Configuration"), isConfPage || isStoragePage);
  //sendTagButton(FS(S_url_storage), F("Storage dump"), isStoragePage);
  sendRawData(F("</form>")); 
  sendRawData(F("<hr/>"));

  if (isRootPage){
    sendStatusPage();
  } 
  //  if (isPinsPage){
  //    sendPinsStatus();
  //  } 
  else if (isLogPage){
    sendLogPage(getParams);
  }
  else if (isConfPage){
    sendConfigurationPage(getParams);
  } 
  else if (isStoragePage){
    sendStorageDumpPage(getParams); 
  }

  if (c_isWifiResponseError) return;

  sendRawData(F("</body></html>"));

  /*   
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
  sendRawData(F(" C</dd>")); 
  sendRawData(F("<dd>Forecast: ")); 
  sendRawData(statisticsTemperature);
  sendRawData(F(" C (")); 
  sendRawData(statisticsCount);
  sendRawData(F(" measurements)</dd>"));


  sendRawData(F("<dt>Logger</dt>"));
  if (GB_StorageHelper.isStoreLogRecordsEnabled()){
    sendRawData(F("<dd>Enabled</dd>"));
  } 
  else {
    sendRawData(F("<dd class='red'>Disabled</dd>"));
  }
  sendRawData(F("<dd>Stored records: "));
  sendRawData(GB_StorageHelper.getLogRecordsCount());
  sendRawData('/');
  sendRawData(GB_StorageHelper.LOG_CAPACITY);
  if (GB_StorageHelper.isLogOverflow()){
    sendRawData(F(", <span class='red'>overflow</span>"));
  }
  sendRawData(F("</dd>"));


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
//                             LOG PAGE                            //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendLogPage(const String& getParams){

  LogRecord logRecord;

  tmElements_t targetTm;
  tmElements_t currentTm;
  tmElements_t previousTm;

  String paramValue; 

  // fill targetTm
  boolean isTargetDayFound = false;
  if (searchHttpParamByName(getParams, F("date"), paramValue)){

    if (paramValue.length() == 10){
      byte dayInt   = paramValue.substring(0, 2).toInt();
      byte monthInt = paramValue.substring(3, 5).toInt();
      word yearInt  = paramValue.substring(6, 10).toInt();

      if (dayInt != 0 && monthInt != 0 && yearInt !=0){
        targetTm.Day = dayInt;
        targetTm.Month = monthInt;
        targetTm.Year = CalendarYrToTm(yearInt);
        isTargetDayFound = true;
      }
    }
  } 
  if (!isTargetDayFound){
    breakTime(now(), targetTm);
  }
  String targetDateAsString = StringUtils::timeStampToString(makeTime(targetTm), true, false);

  boolean printEvents, printErrors, printTemperature;
  printEvents = printErrors = printTemperature = true;

  if (searchHttpParamByName(getParams, F("type"), paramValue)){
    if (StringUtils::flashStringEquals(paramValue, F("events"))){
      printEvents = true; 
      printErrors = false; 
      printTemperature = false;
    } 
    else if (StringUtils::flashStringEquals(paramValue, F("errors"))){
      printEvents = false; 
      printErrors = true; 
      printTemperature = false;
    }
    else if (StringUtils::flashStringEquals(paramValue, F("temperature"))){
      printEvents = false; 
      printErrors = false; 
      printTemperature = true;
    } 
    else{
      printEvents = true; 
      printErrors = true; 
      printTemperature = true;
    }
  } 

  // fill previousTm
  previousTm.Day = previousTm.Month = previousTm.Year = 0; //remove compiller warning

  sendRawData(F("<table style='width:100%'><colgroup><col width='60%'/><col width='40%'/></colgroup><tr><td>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_log));
  sendRawData(F("' method='get'>"));
  sendRawData(F("<select id='typeCombobox' name='type'></select>"));
  sendRawData(F("<select id='dateCombobox' name='date'></select>"));
  sendRawData(F("<input type='submit' value='Show'/>"));
  sendRawData(F("</form>"));
  // out of form
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F(""), F("All types"), printEvents && printErrors && printTemperature);
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F("events"), F("Events only"), printEvents && !printErrors && !printTemperature);
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F("errors"), F("Errors only"), !printEvents && printErrors && !printTemperature);
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F("temperature"), F("Temperature only"), !printEvents && !printErrors && printTemperature);

  sendRawData(F("</td><td style='text-align:right;'>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_log));
  sendRawData(F("' method='post' onSubmit='return confirm(\"Delete all stored records?\")'>"));
  sendRawData(F("Stored records: "));
  sendRawData(GB_StorageHelper.getLogRecordsCount());
  sendRawData('/');
  sendRawData(GB_StorageHelper.LOG_CAPACITY);
  if (GB_StorageHelper.isLogOverflow()){
    sendRawData(F(", <span class='red'>overflow</span>"));
  }
  sendRawData(F("<input type='hidden' name='resetStoredLog'/>"));
  sendRawData(F("<input type='submit' value='Delete all'/>"));
  sendRawData(F("</form>"));
  sendRawData(F("</td></table>"));

  boolean isTableTagPrinted = false;   
  word index;
  for (index = 0; index < GB_StorageHelper.getLogRecordsCount(); index++){

    if (c_isWifiResponseError) return;

    logRecord = GB_StorageHelper.getLogRecordByIndex(index);
    breakTime(logRecord.timeStamp, currentTm);

    boolean isDaySwitch = !(currentTm.Day == previousTm.Day && currentTm.Month == previousTm.Month && currentTm.Year == previousTm.Year);
    if (isDaySwitch){
      previousTm = currentTm; 
    }
    boolean isTargetDay = (currentTm.Day == targetTm.Day && currentTm.Month == targetTm.Month && currentTm.Year == targetTm.Year);  
    String currentDateAsString = StringUtils::timeStampToString(logRecord.timeStamp, true, false);

    if (isDaySwitch){ 
      sendAppendOptionToSelectDynamic(F("dateCombobox"), F(""), currentDateAsString, isTargetDay);
    }

    if (!isTargetDay){
      continue;
    }
    if (!printEvents && GB_Logger.isEvent(logRecord)){
      continue;
    } 
    boolean isError = GB_Logger.isError(logRecord);
    if (!printErrors && isError){
      continue;
    } 
    if (!printTemperature && GB_Logger.isTemperature(logRecord)){
      continue;
    }

    if (!isTableTagPrinted){
      isTableTagPrinted = true;
      sendRawData(F("<table class='grab'>"));
      sendRawData(F("<tr><th>#</th><th>Time</th><th>Description</th></tr>"));
    }
    sendRawData(F("<tr"));
    if (isError){
      sendRawData(F(" class='red'"));
    }
    sendRawData(F("><td>"));
    sendRawData(index+1);
    sendRawData(F("</td><td>"));
    sendRawData(StringUtils::timeStampToString(logRecord.timeStamp, false, true)); 
    sendRawData(F("</td><td style='text-align:left;'>"));
    sendRawData(GB_Logger.getLogRecordDescription(logRecord));
    sendRawData(GB_Logger.getLogRecordDescriptionSuffix(logRecord));
    sendRawData(F("</td></tr>")); // bug with linker was here https://github.com/arduino/Arduino/issues/1071#issuecomment-19832135
  }
  if (isTableTagPrinted) {
    sendRawData(F("</table>"));
  } 
  else {  
    sendRawData(F("No stored log records found during "));
    sendRawData(targetDateAsString);
  }
}

/////////////////////////////////////////////////////////////////////
//                      CONFIGURATION PAGE                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendConfigurationPage(const String& getParams){

  sendRawData(F("<fieldset><legend>Growing</legend>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post'>"));
  sendTagCheckbox(F("isStoreLogRecordsEnabled"), F("Enable logger"), GB_StorageHelper.isStoreLogRecordsEnabled());
  sendRawData(F("<br/><input type='submit' value='Save'>"));
  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));
  sendRawData(F("<br/>"));

  boolean isWifiStationMode = GB_StorageHelper.isWifiStationMode();

  sendRawData(F("<fieldset><legend>Wi-Fi</legend>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post'>"));
  sendTagRadioButton(F("isWifiStationMode"), F("Create new Network"), F("0"),!isWifiStationMode);
  sendRawData(F("<br/>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<small>Hidden, security WPA2, ip 192.168.0.1</small><br/>"));
  sendTagRadioButton(F("isWifiStationMode"), F("Join existed Network"), F("1"), isWifiStationMode);
  sendRawData(F("<table>"));
  sendRawData(F("<tr><td><label for='wifiSSID'>SSID</label></td><td><input type='text' name='wifiSSID' required value='"));
  sendRawData(GB_StorageHelper.getWifiSSID());
  sendRawData(F("' maxlength='"));
  sendRawData(WIFI_SSID_LENGTH);
  sendRawData(F("'/></td></tr>"));
  sendRawData(F("<tr><td><label for='wifiPass'>Password</label></td><td><input type='password' name='wifiPass' required value='"));
  sendRawData(GB_StorageHelper.getWifiPass());
  sendRawData(F("' maxlength='"));
  sendRawData(WIFI_PASS_LENGTH);
  sendRawData(F("'/></td></tr>"));
  sendRawData(F("</table>")); 
  sendRawData(F("<input type='submit' value='Save'><small>and reboot Growbox manually</small>"));
  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));
  sendRawData(F("<br/>"));

  sendRawData(F("<fieldset><legend>Other</legend>"));  
  sendRawData(F("<a href='"));
  sendRawData(FS(S_url_storage));
  sendRawData(F("'>View EEPROM dumps</a><br/><br/>")); 
  sendRawData(F("<table style='vertical-align:top; border-spacing:0px;'><tr><td>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_root));
  sendRawData(F("' method='post' onSubmit='return confirm(\"Reboot Growbox?\")'>"));
  sendRawData(F("<input type='hidden' name='rebootController'/><input type='submit' value='Reboot Growbox'>"));
  sendRawData(F("</td><td>"));
  sendRawData(F("<small> and update page manually</small>"));
  sendRawData(F("</td></tr><tr><td>"));
  sendRawData(F("</form>")); 
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_root));
  sendRawData(F("' method='post' onSubmit='return confirm(\"Are you seriously want to reset Firmware and reboot?\")'>"));
  sendRawData(F("<input type='hidden' name='resetFirmware'/><input type='submit' value='Reset Firmware'>"));
  sendRawData(F("</td><td>"));
  sendRawData(F("<small>and update page manually. Default Wi-Fi SSID and pass will be used</small>"));
  sendRawData(F("</form>"));
  sendRawData(F("</td></tr></table>"));
  sendRawData(F("</fieldset>"));
  sendRawData(F("<br/>"));
}

void WebServerClass::sendStorageDumpPage(const String& getParams){

  String paramValue;
  boolean isArduino = true;
  if (searchHttpParamByName(getParams, F("type"), paramValue)){
    if (paramValue.length()==1){
      if (paramValue[0]=='r'){
        isArduino = false;
      }
    } 
  }
  byte rangeStart=0x0; //0x[0]00
  byte rangeEnd=0x0;   //0x[0]FF
  byte temp;
  if (searchHttpParamByName(getParams, F("rangeStart"), paramValue)){
    if (paramValue.length() == 1) {
      temp = StringUtils::hexCharToByte(paramValue[0]);
      if (temp != 0xFF) {
        rangeStart = temp;
      }
    }
  }
  if (searchHttpParamByName(getParams, F("rangeEnd"), paramValue)){
    if (paramValue.length() == 1) {
      temp = StringUtils::hexCharToByte(paramValue[0]);
      if (temp != 0xFF) {
        rangeEnd = temp;
      }
    }
  }
  if (rangeStart > rangeEnd){
    rangeEnd = rangeStart;
  }

  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_storage));
  sendRawData(F("' method='get' onSubmit='if (document.getElementById(\"rangeStartCombobox\").value > document.getElementById(\"rangeEndCombobox\").value){ alert(\"Address range is incorrect\"); return false;}'>"));
  sendRawData(F("EEPROM Dump for "));
  sendRawData(F("<select id='typeOfStorageCombobox' name='type'></select>"));
  sendRawData(F("<br/>Address from "));
  sendRawData(F("<select id='rangeStartCombobox' name='rangeStart'></select>"));
  sendRawData(F(" to "));
  sendRawData(F("<select id='rangeEndCombobox' name='rangeEnd'></select>"));
  sendRawData(F("<input type='submit' value='Show'/>"));
  sendRawData(F("</form>"));

  // out of form
  sendAppendOptionToSelectDynamic(F("typeOfStorageCombobox"), F("a"), F("Arduino (internal)"), isArduino);
  sendAppendOptionToSelectDynamic(F("typeOfStorageCombobox"), F("r"), F("RTC (external)"), !isArduino);

  String stringValue;
  for (byte counter = 0; counter< 0x10; counter++){
    stringValue = StringUtils::byteToHexString(counter, true);
    stringValue += '0';
    stringValue += '0';
    sendAppendOptionToSelectDynamic(F("rangeStartCombobox"), String(counter, HEX), stringValue, rangeStart==counter);
  }

  for (byte counter = 0; counter< 0x10; counter++){
    stringValue = StringUtils::byteToHexString(counter, true);
    stringValue += 'F';
    stringValue += 'F';
    sendAppendOptionToSelectDynamic(F("rangeEndCombobox"), String(counter, HEX), stringValue, rangeEnd==counter);
  }

  sendRawData(F("<table class='grab'><tr><th/>"));
  for (word i = 0; i < 0x10 ; i++){
    sendRawData(F("<th>"));
    String colName(i, HEX);
    colName.toUpperCase();
    sendRawData(colName);
    sendRawData(F("</th>"));
  }
  sendRawData(F("</tr>"));
  
  word realRangeStart = ((word)(rangeStart)<<8);
  word realRangeEnd   = ((word)(rangeEnd)<<8) + 0xFF;
  byte value; 
  for (word i = realRangeStart; i <= realRangeEnd/*EEPROM_AT24C32.getCapacity()*/ ; i++){

    if (c_isWifiResponseError) return;

    if (isArduino){
      value = EEPROM.read(i);
    } 
    else {
      value = EEPROM_AT24C32.read(i);
    }

    if ( i%16 == 0){
      if (i > 0){
        sendRawData(F("</tr>"));
      }
      sendRawData(F("<tr><td><b>"));
      sendRawData(StringUtils::byteToHexString(i/16));
      sendRawData(F("</b></td>"));
    }
    sendRawData(F("<td>"));
    sendRawData(StringUtils::byteToHexString(value));
    sendRawData(F("</td>"));

  }
  sendRawData(F("</tr></table>"));
}


/////////////////////////////////////////////////////////////////////
//                          OTHER PAGES                            //
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
//                          POST HANDLING                          //
/////////////////////////////////////////////////////////////////////

String WebServerClass::applyPostParams(const String& url, const String& postParams){
  //String queryStr;
  word index = 0;  
  String name, value;
  while(getHttpParamByIndex(postParams, index, name, value)){
    //if (queryStr.length() > 0){
    //  queryStr += '&';
    //}
    //queryStr += name;
    //queryStr += '=';   
    //queryStr += applyPostParam(name, value);
    applyPostParam(name, value);
    index++;
  }
  //if (index > 0){
  //  return url + String('?') + queryStr;
  //}
  //else {
  return url;
  //}
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
  else if (StringUtils::flashStringEquals(name, F("rebootController"))){
    GB_Controller.rebootController();

  } 
  else if (StringUtils::flashStringEquals(name, F("resetStoredLog"))){
    GB_StorageHelper.resetStoredLog();
    GB_Controller.rebootController();

  } 
  else if (StringUtils::flashStringEquals(name, F("isStoreLogRecordsEnabled"))){
    if (value.length() != 1){
      return false;
    }
    GB_StorageHelper.setStoreLogRecordsEnabled(value[0]=='1'); 
  }
  else if (StringUtils::flashStringEquals(name, F("resetFirmware"))){
    GB_StorageHelper.resetFirmware(); 
  }
  else {
    return false;
  }

  return true;
}

WebServerClass GB_WebServer;




