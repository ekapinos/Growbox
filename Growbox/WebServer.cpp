#include "WebServer.h"

#include <MemoryFree.h>
#include "Controller.h"
#include "StorageHelper.h" 
#include "Thermometer.h" 
#include "Watering.h"
#include "RAK410_XBeeWifi.h" 
#include "EEPROM_AT24C32.h" 

void WebServerClass::init(){ 
  RAK410_XBeeWifi.init();
}

void WebServerClass::update(){
  RAK410_XBeeWifi.update();
}

boolean WebServerClass::handleSerialEvent(){

  String url, getParams, postParams; 

  // HTTP response supplemental   
  RAK410_XBeeWifiClass::RequestType commandType = RAK410_XBeeWifi.handleSerialEvent(c_wifiPortDescriptor, url, getParams, postParams);

  c_isWifiResponseError = false;
  c_isWifiForceUpdateGrowboxState = false;

  switch(commandType){
    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET:
    if (StringUtils::flashStringEquals(url, FS(S_url_root)) || 
      StringUtils::flashStringEquals(url, FS(S_url_log)) ||
      StringUtils::flashStringStartsWith(url, FS(S_url_watering)) ||
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
  return c_isWifiForceUpdateGrowboxState;
}

boolean WebServerClass::handleSerialMonitorEvent(){

  String input(Serial.readString());

  if (StringUtils::flashStringEndsWith(input, FS(S_CRLF))){
    input = input.substring(0, input.length()-2);   
  }

  int indexOfQestionChar = input.indexOf('?');
  if (indexOfQestionChar < 0){
    if(g_useSerialMonitor){ 
      Serial.print(F("Wrong serial command ["));
      Serial.print(input);
      Serial.print(F("]. '?' char not found. Syntax: url?param1=value1[&param2=value]"));
    }
    return false;
  }

  String url(input.substring(0, indexOfQestionChar));
  String postParams(input.substring(indexOfQestionChar+1));

  if(g_useSerialMonitor){ 
    Serial.print(F("Recive from [SM] POST ["));
    Serial.print(url);
    Serial.print(F("] postParams ["));
    Serial.print(postParams);   
    Serial.println(']');
  }

  c_isWifiForceUpdateGrowboxState = false;
  applyPostParams(url, postParams);
  return c_isWifiForceUpdateGrowboxState;
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

void WebServerClass::sendRawData(time_t data, boolean interpretateAsULong){
  if (interpretateAsULong == true){
    String str(data);
    sendRawData(str);
  } 
  else {
    if (data == 0){
      sendRawData(F("N/A"));
    } 
    else {
      String str = StringUtils::timeStampToString(data);
      sendRawData(str);
    }
  }
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

void WebServerClass::sendTagInputNumber(const __FlashStringHelper* name, const __FlashStringHelper* text, word minValue, word maxValue, word value){
  if (text != 0){
    sendRawData(F("<label>"));
    sendRawData(text);
  }
  sendRawData(F("<input type='number' required='required' name='"));
  sendRawData(name);
  sendRawData(F("' id='"));
  sendRawData(name);
  sendRawData(F("' min='"));
  sendRawData(minValue);
  sendRawData(F("' max='"));
  sendRawData(maxValue);
  sendRawData(F("' value='"));
  sendRawData(value);
  sendRawData(F("'/>")); 
  if (text != 0){
    sendRawData(F("</label>")); 
  }
} 

void WebServerClass::sendTagInputTime(const __FlashStringHelper* name, const __FlashStringHelper* text, word value){
  if (text != 0){
    sendRawData(F("<label>"));
    sendRawData(text);
  }
  sendRawData(F("<input type='time' required='required' step='60' name='"));
  sendRawData(name);
  sendRawData(F("' id='"));
  sendRawData(name);
  sendRawData(F("' value='"));
  sendRawData(StringUtils::wordTimeToString(value));
  sendRawData(F("'/>")); 
  if (text != 0){
    sendRawData(F("</label>")); 
  }
}   

word WebServerClass::getTimeFromInput(const String& value){
  if (value.length() != 5){
    return 0xFFFF;
  }

  if (value[2] != ':'){
    return 0xFFFF;
  }

  byte valueHour   = (value[0]-'0') * 10 + (value[1]-'0');
  byte valueMinute = (value[3]-'0') * 10 + (value[4]-'0');
  return valueHour*60+valueMinute;
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

void WebServerClass::sendTimeStampJavaScript(const __FlashStringHelper* growboxTimeStampId, const __FlashStringHelper* browserTimeStampId, const __FlashStringHelper* diffTimeStampId){

  sendRawData(F("<script type='text/javascript'>"));
  sendRawData(F("var g_timeFormat = {year: 'numeric', month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', second: '2-digit'};"));
  sendRawData(F("var g_growboxTimeStamp = new Date(("));
  sendRawData(now() + UPDATE_WEB_SERVER_AVERAGE_PAGE_LOAD_DELAY, true);
  sendRawData(F(" + new Date().getTimezoneOffset()*60) * 1000"));
  //  tmElements_t nowTmElements; 
  //  breakTime(now(), nowTmElements);
  //  sendRawData(tmYearToCalendar(nowTmElements.Year));
  //  sendRawData(',');
  //  sendRawData(nowTmElements.Month-1);
  //  sendRawData(',');
  //  sendRawData(nowTmElements.Day);
  //  sendRawData(',');
  //  sendRawData(nowTmElements.Hour);
  //  sendRawData(',');
  //  sendRawData(nowTmElements.Minute);
  //  sendRawData(',');
  //  sendRawData(nowTmElements.Second);
  sendRawData(F(");"));
  sendRawData(F("var g_timeStampDiff = new Date().getTime() - g_growboxTimeStamp;"));
  if (diffTimeStampId != 0){
    sendRawData(F("var absDiffInSeconds = Math.abs(Math.floor(g_timeStampDiff/1000));"));    
    sendRawData(F("var diffSeconds = absDiffInSeconds % 60;"));    
    sendRawData(F("var diffMinutes = Math.floor(absDiffInSeconds/60) % 60;"));    
    sendRawData(F("var diffHours = Math.floor(absDiffInSeconds/60/60);"));    
    sendRawData(F("var resultDiffString;"));    
    sendRawData(F("if (diffHours > 365*24) {"));    
    sendRawData(F("  resultDiffString = 'over year';"));    
    sendRawData(F("} else if (diffHours > 30*24) {"));    
    sendRawData(F("  resultDiffString = 'over month';"));    
    sendRawData(F("} else if (diffHours > 7*24) {"));    
    sendRawData(F("  resultDiffString = 'over week';"));    
    sendRawData(F("} else if (diffHours > 24) {"));    
    sendRawData(F("  resultDiffString = 'over day';"));    
    sendRawData(F("} else if (diffHours > 0) {"));    
    sendRawData(F("  resultDiffString = diffHours + ' h ' + diffMinutes + ' m ' + diffSeconds + ' s';"));    
    sendRawData(F("} else if (diffMinutes > 0) {"));    
    sendRawData(F("  resultDiffString = diffMinutes + ' min ' + diffSeconds + ' sec';"));    
    sendRawData(F("} else if (diffSeconds > 0) {"));    
    sendRawData(F("  resultDiffString = diffSeconds + ' sec';"));    
    sendRawData(F("} else {"));    
    sendRawData(F("  resultDiffString = '';"));    
    sendRawData(F("}"));  

    sendRawData(F("var diffSpan = document.getElementById('"));    
    sendRawData(diffTimeStampId);    
    sendRawData(F("');"));   

    sendRawData(F("diffSpan.innerHTML = \""));   
    if (GB_StorageHelper.isClockTimeStampAutoCalculated()) {
      sendTextRedIfTrue(F("Auto calculated. "), true);
    }
    sendRawData(F("\";"));   
    sendRawData(F("if (absDiffInSeconds == 0) {")); 
    sendRawData(F("  diffSpan.innerHTML += 'Synced with browser time'"));
    sendRawData(F("} else {")); 
    sendRawData(F("  diffSpan.innerHTML += 'Out of sync ' + resultDiffString + ' with browser time';"));   
    sendRawData(F("}")); 

    sendRawData(F("if (diffHours > 0 || diffMinutes > 0) {"));  
    sendRawData(F("  diffSpan.style.color='red';"));  
    sendRawData(F("}"));  
  }  
  sendRawData(F("function updateTimeStamps() {"));
  sendRawData(F("    var browserTimeStamp = new Date();"));
  sendRawData(F("    g_growboxTimeStamp.setTime(browserTimeStamp.getTime() - g_timeStampDiff);"));
  if (growboxTimeStampId != 0){
    sendRawData(F("document.getElementById('"));
    sendRawData(growboxTimeStampId);
    sendRawData(F("').innerHTML = g_growboxTimeStamp.toLocaleString('uk', g_timeFormat);"));
  }
  if (browserTimeStampId != 0){
    sendRawData(F("document.getElementById('"));
    sendRawData(browserTimeStampId);
    sendRawData(F("').innerHTML = browserTimeStamp.toLocaleString('uk', g_timeFormat);"));
  }
  sendRawData(F("    setTimeout(function () {updateTimeStamps(); }, 1000);"));
  sendRawData(F("};"));
  sendRawData(F("updateTimeStamps();"));
  sendRawData(F("</script>"));
}

void WebServerClass::sendTextRedIfTrue(const __FlashStringHelper* text, boolean isRed){
  if (isRed){
    sendRawData(F("<span class='red'>")); 
  } 
  else {
    sendRawData(F("<span>")); 
  }
  sendRawData(text);
  sendRawData(F("</span>"));
}

/////////////////////////////////////////////////////////////////////
//                        COMMON FOR PAGES                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendHttpPageBody(const String& url, const String& getParams){

  boolean isRootPage    = StringUtils::flashStringEquals(url, FS(S_url_root));
  boolean isLogPage     = StringUtils::flashStringEquals(url, FS(S_url_log));
  boolean isWateringPage= StringUtils::flashStringStartsWith(url, FS(S_url_watering));
  boolean isConfPage    = StringUtils::flashStringEquals(url, FS(S_url_configuration));
  boolean isStoragePage = StringUtils::flashStringEquals(url, FS(S_url_storage));

  if (isRootPage || isWateringPage) {
    GB_Watering.turnOnWetSensors();
  }

  sendRawData(F("<!DOCTYPE html>")); // HTML 5
  sendRawData(F("<html><head>"));
  sendRawData(F("  <title>Growbox</title>"));
  sendRawData(F("  <meta name='viewport' content='width=device-width, initial-scale=1'/>"));  
  sendRawData(F("<style type='text/css'>"));
  sendRawData(F("  body {font-family:Arial; max-width:600px; }")); // 350px 
  sendRawData(F("  form {margin: 0px;}"));
  sendRawData(F("  dt {font-weight: bold; margin-top:5px;}")); 

  sendRawData(F("  .red {color: red;}"));
  sendRawData(F("  .grab {border-spacing:5px; width:100%; }")); 
  sendRawData(F("  .align_right {float:right; text-align:right;}"));
  sendRawData(F("  .align_center {float:center; text-align:center;}")); 
  sendRawData(F("  .description {font-size:small; margin-left:20px; margin-bottom:5px;}")); 
  sendRawData(F("</style>"));  
  sendRawData(F("</head>"));

  sendRawData(F("<body>"));
  sendRawData(F("<h1>Growbox</h1>"));   
  sendRawData(F("<form>"));   // HTML Validator warning
  sendTagButton(FS(S_url_root), F("Status"), isRootPage);
  sendTagButton(FS(S_url_log), F("Daily log"), isLogPage);
  sendTagButton(FS(S_url_watering), F("Watering"), isWateringPage);  
  sendTagButton(FS(S_url_configuration), F("Configuration"), isConfPage || isStoragePage);
  sendRawData(F("</form>")); 
  sendRawData(F("<hr/>"));

  if (isRootPage){
    sendStatusPage();
  } 
  if (isWateringPage){
    sendWateringPage(url);
  } 
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

}

/////////////////////////////////////////////////////////////////////
//                          STATUS PAGE                            //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendStatusPage(){


  sendRawData(F("<dl>"));

  sendRawData(F("<dt>General</dt>"));
  if (!GB_Controller.isHardwareClockPresent()) {
    sendRawData(F("<dd>Clock: "));
    sendTextRedIfTrue(F("Not connected"), true);
    sendRawData(F("</dd>")); 
  }
  sendRawData(F("<dd>")); 
  sendRawData(g_isDayInGrowbox ? F("Day") : F("Night"));
  sendRawData(F(" mode</dd>"));
  sendRawData(F("<dd>Light: ")); 
  sendRawData((digitalRead(LIGHT_PIN) == RELAY_ON) ? F("Enabled") : F("Disabled"));
  sendRawData(F("</dd>"));
  sendRawData(F("<dd>Fan: ")); 
  boolean isFanEnabled = (digitalRead(FAN_PIN) == RELAY_ON);
  sendRawData(isFanEnabled ? F("Enabled, ") : F("Disabled"));
  if (isFanEnabled){
    sendRawData((digitalRead(FAN_SPEED_PIN) == FAN_SPEED_MIN) ? F("min speed") : F("max speed"));
  }
  sendRawData(F("</dd>"));
  sendRawData(F("<dd>Last startup: ")); 
  sendRawData(GB_StorageHelper.getLastStartupTimeStamp());
  sendRawData(F("</dd>")); 
  sendRawData(F("<dd>Current time: ")); 
  sendRawData(F("<span id='growboxTimeStampId'></span><small><br/><span id='diffTimeStampId'></span></small>"));
  sendTimeStampJavaScript(F("growboxTimeStampId"), 0, F("diffTimeStampId"));
  sendRawData(F("</dd>")); 

  if (c_isWifiResponseError) return;

  float lastTemperature, statisticsTemperature;
  int statisticsCount;
  GB_Thermometer.getStatistics(lastTemperature, statisticsTemperature, statisticsCount);

  byte normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue;
  GB_StorageHelper.getTemperatureParameters(normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue);


  sendRawData(F("<dt>Temperature</dt>"));   
  if (!GB_Thermometer.isPresent()){
    sendTextRedIfTrue(F("<dd>Not connected</dd>"), true);
  }
  sendRawData(F("<dd>Last check: "));
  if (isnan(lastTemperature)){
    sendRawData(F("N/A"));
  } 
  else {
    sendRawData(lastTemperature);
    sendRawData(F(" &deg;C")); 
  }
  sendRawData(F("</dd>")); 
  sendRawData(F("<dd>Forecast: ")); 
  if (isnan(statisticsTemperature)){
    sendRawData(F("N/A"));
  } 
  else {
    sendRawData(statisticsTemperature);
    sendRawData(F(" &deg;C")); 
  }
  sendRawData(F(" ("));
  sendRawData(statisticsCount);
  sendRawData(F(" measurement"));
  if (statisticsCount > 1){
    sendRawData('s');
  }
  sendRawData(F(")</dd>"));

  if (c_isWifiResponseError) return;

  if(g_useSerialMonitor){ 
    Serial.println();
  }
  GB_Watering.turnOffWetSensorsAndUpdateWetStatus();
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

    if (!wsp.boolPreferencies.isWetSensorConnected && !wsp.boolPreferencies.isWaterPumpConnected){
      continue;
    }

    sendRawData(F("<dt>Watering system #"));
    sendRawData(wsIndex+1); 
    sendRawData(F("</dt>")); 

    if (wsp.boolPreferencies.isWetSensorConnected) {
      sendRawData(F("<dd>Wet sensor: "));
      WateringEvent* currentStatus = GB_Watering.getCurrentWetSensorStatus(wsIndex);     
      sendTextRedIfTrue(currentStatus->shortDescription, currentStatus != &WATERING_EVENT_WET_SENSOR_NORMAL);
      sendRawData(F("</dd>"));
    }

    if (wsp.boolPreferencies.isWaterPumpConnected) {
      sendRawData(F("<dd>Last watering: "));
      sendRawData(GB_Watering.getLastWateringTimeStampByIndex(wsIndex));
      sendRawData(F("</dd>"));

      sendRawData(F("<dd>Next watering: "));
      sendRawData(GB_Watering.getNextWateringTimeStampByIndex(wsIndex));
      sendRawData(F("</dd>")); 
    }
  }

  sendRawData(F("<dt>Logger</dt>"));
  if (GB_StorageHelper.isUseExternal_EEPROM_AT24C32() && !EEPROM_AT24C32.isPresent()) {
    sendRawData(F("<dd>External AT24C32 EEPROM: "));
    sendTextRedIfTrue(F("Not connected"), true);
    sendRawData(F("</dd>")); 
  } else if (!GB_StorageHelper.isUseExternal_EEPROM_AT24C32() && EEPROM_AT24C32.isPresent()) {
    sendRawData(F("<dd>External AT24C32 EEPROM: "));
    sendRawData(F("Connected, not used"));
    sendRawData(F("</dd>")); 
  }
  
  if (!GB_StorageHelper.isStoreLogRecordsEnabled()){
    sendRawData(F("<dd>")); 
    sendTextRedIfTrue(F("Disabled"), true);
    sendRawData(F("</dd>")); 
  }
  sendRawData(F("<dd>Stored records: "));
  sendRawData(GB_StorageHelper.getLogRecordsCount());
  sendRawData('/');
  sendRawData(GB_StorageHelper.getLogRecordsCapacity());
  if (GB_StorageHelper.isLogOverflow()){
    sendRawData(F(", "));
    sendTextRedIfTrue(F("overflow"), true);
  }
  sendRawData(F("</dd>"));

  if (c_isWifiResponseError) return;

  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  sendRawData(F("<dt>Configuration</dt>"));  
  sendRawData(F("<dd>Turn to Day mode at: ")); 
  sendRawData(StringUtils::wordTimeToString(upTime));
  sendRawData(F("</dd><dd>Turn to Night mode at: "));
  sendRawData(StringUtils::wordTimeToString(downTime));
  sendRawData(F("</dd>"));

  sendRawData(F("<dd>Day temperature: "));
  sendRawData(normalTemperatueDayMin);
  sendRawData(F(".."));
  sendRawData(normalTemperatueDayMax);
  sendRawData(F(" &deg;C</dd><dd>Night temperature: "));
  sendRawData(normalTemperatueNightMin);
  sendRawData(F(".."));
  sendRawData(normalTemperatueNightMax);
  sendRawData(F(" &deg;C</dd><dd>Critical temperature: "));
  sendRawData(criticalTemperatue);
  sendRawData(F(" &deg;C</dd>"));

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

  boolean printEvents, printWateringEvents, printErrors, printTemperature;
  printEvents = printWateringEvents = printErrors = printTemperature = true;

  if (searchHttpParamByName(getParams, F("type"), paramValue)){
    if (StringUtils::flashStringEquals(paramValue, F("events"))){
      printEvents = true; 
      printWateringEvents = false;
      printErrors = false; 
      printTemperature = false;
    } 
    if (StringUtils::flashStringEquals(paramValue, F("wateringevents"))){
      printEvents = false; 
      printWateringEvents = true;
      printErrors = false; 
      printTemperature = false;
    } 
    else if (StringUtils::flashStringEquals(paramValue, F("errors"))){
      printEvents = false; 
      printWateringEvents = false;
      printErrors = true; 
      printTemperature = false;
    }
    else if (StringUtils::flashStringEquals(paramValue, F("temperature"))){
      printEvents = false; 
      printWateringEvents = false;
      printErrors = false; 
      printTemperature = true;
    } 
    else{
      printEvents = true; 
      printWateringEvents = true;
      printErrors = true; 
      printTemperature = true;
    }
  } 

  if (c_isWifiResponseError) return;

  // fill previousTm
  previousTm.Day = previousTm.Month = previousTm.Year = 0; //remove compiller warning

  sendRawData(F("<table class='grab'><colgroup><col width='60%'/><col width='40%'/></colgroup><tr><td>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_log));
  sendRawData(F("' method='get'>"));
  sendRawData(F("<select id='typeCombobox' name='type'></select>"));
  sendRawData(F("<select id='dateCombobox' name='date'></select>"));
  sendRawData(F("<input type='submit' value='Show'/>"));
  sendRawData(F("</form>"));

  sendRawData(F("</td><td class='align_right'>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_log));
  sendRawData(F("' method='post' onSubmit='return confirm(\"Delete all stored records?\")'>"));
  sendRawData(F("Stored records: "));
  sendRawData(GB_StorageHelper.getLogRecordsCount());
  sendRawData('/');
  sendRawData(GB_StorageHelper.getLogRecordsCapacity());
  if (GB_StorageHelper.isLogOverflow()){
    sendRawData(F(", "));
    sendTextRedIfTrue(F("overflow"), true);
  }
  sendRawData(F("<input type='hidden' name='resetStoredLog'/>"));
  sendRawData(F("<input type='submit' value='Reset log'/>"));
  sendRawData(F("</form>"));
  sendRawData(F("</td></table>"));

  // out of form and table
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F("all"), F("All records"),                      printEvents &&  printWateringEvents &&  printErrors &&  printTemperature);
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F("events"), F("Events only"),                   printEvents && !printWateringEvents && !printErrors && !printTemperature);
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F("wateringevents"), F("Watering Events only"), !printEvents &&  printWateringEvents && !printErrors && !printTemperature);
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F("errors"), F("Errors only"),                  !printEvents && !printWateringEvents &&  printErrors && !printTemperature);
  sendAppendOptionToSelectDynamic(F("typeCombobox"), F("temperature"), F("Temperature only"),        !printEvents && !printWateringEvents && !printErrors &&  printTemperature);

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
    
    boolean isEvent         = GB_Logger.isEvent(logRecord);
    boolean isWateringEvent = GB_Logger.isWateringEvent(logRecord);
    boolean isError         = GB_Logger.isError(logRecord);
    boolean isTemperature   = GB_Logger.isTemperature(logRecord);
    
    if (!printEvents && isEvent){
      continue;
    } 
    if (!printWateringEvents && isWateringEvent){
      continue;
    }
    if (!printErrors && isError){
      continue;
    } 
    if (!printTemperature && isTemperature){
      continue;
    }

    if (!isTableTagPrinted){
      isTableTagPrinted = true;
      sendRawData(F("<table class='grab align_center'>"));
      sendRawData(F("<tr><th>#</th><th>Time</th><th>Description</th></tr>"));
    }
    sendRawData(F("<tr"));
    if (isError || logRecord.isEmpty()){
      sendRawData(F(" class='red'"));
    } 
    else if (isEvent && (logRecord.data == EVENT_FIRST_START_UP.index || logRecord.data == EVENT_RESTART.index)){ // TODO create check method in Logger.h  
      sendRawData(F(" style='font-weight:bold;'"));
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
    sendRawData(F("No stored Log Records found during "));
    sendRawData(targetDateAsString);
  }
}

/////////////////////////////////////////////////////////////////////
//                         WATERING PAGE                           //
/////////////////////////////////////////////////////////////////////

byte WebServerClass::getWateringIndexFromUrl(const String& url){

  if (!StringUtils::flashStringStartsWith(url, FS(S_url_watering))){
    return 0xFF;
  }

  String urlSuffix = url.substring(StringUtils::flashStringLength(FS(S_url_watering)));
  if (urlSuffix.length() == 0){
    return 0;
  }

  urlSuffix = urlSuffix.substring(urlSuffix.length() - 1);     // remove left slash

  byte wateringSystemIndex = urlSuffix.toInt();
  wateringSystemIndex--;
  if (wateringSystemIndex < 0 || wateringSystemIndex >= MAX_WATERING_SYSTEMS_COUNT){
    return 0xFF;
  }
  return wateringSystemIndex;
}

void WebServerClass::sendWateringPage(const String& url){

  byte wsIndex = getWateringIndexFromUrl(url);
  if (wsIndex == 0xFF){
    wsIndex = 0;
  }

  sendRawData(F("<form>"));
  sendRawData(F("Configure watering system <select id='wsIndexCombobox'></select>"));
  sendRawData(F("<input type='button' value='Show' onclick='document.location=document.getElementById(\"wsIndexCombobox\").value'>"));
  sendRawData(F("</form>"));

  String actionURL;
  actionURL += StringUtils::flashStringLoad(FS(S_url_watering));
  if (wsIndex > 0){
    actionURL += '/';
    actionURL += (wsIndex+1);
  }

  // out of form 
  for (byte i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++){
    String url = StringUtils::flashStringLoad(FS(S_url_watering));
    if (i > 0){
      url += '/';
      url += (i+1);
    }
    String description = StringUtils::flashStringLoad(F("# "));
    description += (i+1);
    sendAppendOptionToSelectDynamic(F("wsIndexCombobox"), url, description, (wsIndex==i));
  }


  BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

  sendRawData(F("<br/><big><b>Watering system # ")); 
  sendRawData(wsIndex+1);  
  sendRawData(F(" preferences</b></big><br/><br/>"));

  // run Dry watering form
  sendRawData(F("<form action='"));
  sendRawData(actionURL); 
  sendRawData(F("' method='post' id='runDryWateringNowForm' onSubmit='return confirm(\"Start manually Dry watering during "));
  sendRawData(wsp.dryWateringDuration);
  sendRawData(F(" sec ?\")'>"));
  sendRawData(F("<input type='hidden' name='runDryWateringNow'>"));
  sendRawData(F("</form>"));  

  // clearLastWateringTimeForm
  sendRawData(F("<form action='"));
  sendRawData(actionURL); 
  sendRawData(F("' method='post' id='clearLastWateringTimeForm' onSubmit='return confirm(\"Clear last watering time?\")'>"));
  sendRawData(F("<input type='hidden' name='clearLastWateringTime'>"));
  sendRawData(F("</form>"));

  //  if(g_useSerialMonitor){ 
  //    Serial.println();
  //  }
  GB_Watering.turnOffWetSensorsAndUpdateWetStatus();
  byte currentValue = GB_Watering.getCurrentWetSensorValue(wsIndex);
  WateringEvent*  currentStatus = GB_Watering.getCurrentWetSensorStatus(wsIndex);

  // General form
  if (c_isWifiResponseError) return;

  sendRawData(F("<fieldset><legend>General</legend>"));
  sendRawData(F("<form action='"));
  sendRawData(actionURL);
  sendRawData(F("' method='post'>"));

  sendTagCheckbox(F("isWetSensorConnected"), F("Wet sensor connected"), wsp.boolPreferencies.isWetSensorConnected);

  sendRawData(F("<div class='description'>Current value [<b>"));
  if (GB_Watering.isWetSensorValueReserved(currentValue)){
    sendRawData(F("N/A"));
  } 
  else {
    sendRawData(currentValue);
  }
  sendRawData(F("</b>], state [<b>"));
  sendRawData(currentStatus->shortDescription);
  sendRawData(F("</b>]</div>"));

  sendTagCheckbox(F("isWaterPumpConnected"), F("Watering Pump connected"), wsp.boolPreferencies.isWaterPumpConnected);  
  sendRawData(F("<div class='description'>Last watering&emsp;"));
  sendRawData(GB_Watering.getLastWateringTimeStampByIndex(wsIndex));
  sendRawData(F("<br/>Current time &nbsp;&emsp;"));
  sendRawData(now());
  sendRawData(F("<br/>"));
  sendRawData(F("Next watering&emsp;"));
  sendRawData(GB_Watering.getNextWateringTimeStampByIndex(wsIndex));
  sendRawData(F("</div>"));

  sendRawData(F("<table><tr><td>"));
  sendRawData(F("Every day watering at"));
  sendRawData(F("</td><td>"));
  sendTagInputTime(F("startWateringAt"), 0, wsp.startWateringAt);
  sendRawData(F("</td></tr></table>"));

  sendTagCheckbox(F("useWetSensorForWatering"), F("Use Wet sensor to detect State"), wsp.boolPreferencies.useWetSensorForWatering);
  sendRawData(F("<div class='description'>If Wet sensor [Not connected], [In Air], [Disabled] or [Unstable], then [Dry watering] duration will be used</div>"));

  sendTagCheckbox(F("skipNextWatering"), F("Skip next scheduled watering"), wsp.boolPreferencies.skipNextWatering);
  sendRawData(F("<div class='description'>Useful after manual watering</div>"));

  //sendRawData(F("<br/>"));

  sendRawData(F("<table class='grab'>"));
  sendRawData(F("<tr><td>"));
  sendRawData(F("<input type='submit' value='Save'></td><td class='align_right'>"));
  sendRawData(F("<input type='submit' form='clearLastWateringTimeForm' value='Clear last watering'>"));
  sendRawData(F("<br><input type='submit' form='runDryWateringNowForm' value='Start watering now'>"));
  sendRawData(F("</td></tr>"));
  sendRawData(F("</table>"));

  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));

  sendRawData(F("<br/>"));

  // Wet sensor
  if (c_isWifiResponseError) return;

  sendRawData(F("<fieldset><legend>Wet sensor</legend>"));
  sendRawData(F("<form action='"));
  sendRawData(actionURL);
  sendRawData(F("' method='post'>"));

  sendRawData(F("<table class='grab'>"));

  sendRawData(F("<tr><th>State</th><th>Value range</th></tr>"));

  sendRawData(F("<tr><td>In Air</td><td>"));
  sendTagInputNumber(F("inAirValue"), 0, 1, 255, wsp.inAirValue);
  sendRawData(F(" .. [255]</td></tr>"));

  sendRawData(F("<tr><td>Very Dry</td><td>"));
  sendTagInputNumber(F("veryDryValue"), 0, 1, 255, wsp.veryDryValue);
  sendRawData(F(" .. [In Air]</td></tr>"));

  sendRawData(F("<tr><td>Dry</td><td>"));
  sendTagInputNumber(F("dryValue"), 0, 1, 255, wsp.dryValue);
  sendRawData(F(" .. [Very Dry]</td></tr>"));

  sendRawData(F("<tr><td>Normal</td><td>"));
  sendTagInputNumber(F("normalValue"), 0, 1, 255, wsp.normalValue);
  sendRawData(F(" .. [Dry]</td></tr>"));

  sendRawData(F("<tr><td>Wet</td><td>"));
  sendTagInputNumber(F("wetValue"), 0, 1, 255, wsp.wetValue);
  sendRawData(F(" .. [Normal]</td></tr>"));

  sendRawData(F("<tr><td>Very Wet</td><td>"));
  sendTagInputNumber(F("veryWetValue"), 0, 1, 255, wsp.veryWetValue);
  sendRawData(F(" .. [Wet]</td></tr>"));

  sendRawData(F("<tr><td>Short circit</td><td>[0] .. [Very Wet]</td></tr>"));

  sendRawData(F("</table>"));
  sendRawData(F("<br/><input type='submit' value='Save'>"));
  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));

  sendRawData(F("<br/>"));

  //Watering pump
  if (c_isWifiResponseError) return;

  sendRawData(F("<fieldset><legend>Watering pump</legend>"));

  sendRawData(F("<form action='"));
  sendRawData(actionURL);
  sendRawData(F("' method='post'>"));

  sendRawData(F("<table class='grab'>")); 

  sendRawData(F("<tr><th>State</th><th>Duration</th></tr>"));

  sendRawData(F("<tr><td>Dry watering</td><td>"));
  sendTagInputNumber(F("dryWateringDuration"), 0, 1, 255, wsp.dryWateringDuration);
  sendRawData(F(" sec</td></tr>"));

  sendRawData(F("<tr><td>Very Dry watering</td><td>"));
  sendTagInputNumber(F("veryDryWateringDuration"), 0, 1, 255, wsp.veryDryWateringDuration);
  sendRawData(F(" sec</td></tr>"));

  sendRawData(F("</table>"));
  sendRawData(F("<br/><input type='submit' value='Save'>"));
  sendRawData(F("</form>"));

  sendRawData(F("</fieldset>"));

}

/////////////////////////////////////////////////////////////////////
//                      CONFIGURATION PAGE                         //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendConfigurationPage(const String& getParams){

  sendRawData(F("<fieldset><legend>Date & time</legend>"));

  sendRawData(F("<table class='grab'>"));
  sendRawData(F("<tr><th>Device</th><th>Date & Time</th></tr>"));
  sendRawData(F("<tr><td>This browser</td><td><span id='browserTimeStampId'></span></td></tr>"));
  sendRawData(F("<tr><td>Growbox</td><td><span id='growboxTimeStampId'></span></td></tr>"));
  sendRawData(F("<tr><td></td><td><small><span id='diffTimeStampId'></span></small></td></tr>")); 
  sendRawData(F("</table>"));

  sendTimeStampJavaScript(F("growboxTimeStampId"), F("browserTimeStampId"), F("diffTimeStampId"));
  sendRawData(F("<script type='text/javascript'>"));
  sendRawData(F("var g_checkGrowboxTimeStamp = function () {"));
  sendRawData(F("  if(!confirm(\"Syncronize Growbox time with browser time?\")) {"));
  sendRawData(F("    return false;"));
  sendRawData(F("  }"));
  sendRawData(F("  document.getElementById(\"setClockTimeInput\").value = Math.floor(new Date().getTime()/1000 - new Date().getTimezoneOffset()*60);"));
  sendRawData(F("  return true;"));
  sendRawData(F("}"));
  sendRawData(F("</script>"));

  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post' onSubmit='return g_checkGrowboxTimeStamp()'>"));
  sendRawData(F("<input type='hidden' name='setClockTime' id='setClockTimeInput'>"));
  sendRawData(F("<input type='submit' value='Sync'>"));
  sendRawData(F("</form>"));
  
  sendRawData(F("</fieldset>"));
  
  sendRawData(F("<br/>"));
  if (c_isWifiResponseError) return;
  
  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  byte normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue;
  GB_StorageHelper.getTemperatureParameters(normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue);

  sendRawData(F("<fieldset><legend>Scheduler</legend>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post' onSubmit='if (document.getElementById(\"turnToDayModeAt\").value == document.getElementById(\"turnToNightModeAt\").value { alert(\"Turn times should be different\"); return false;}'>"));
  sendRawData(F("<table>"));
  sendRawData(F("<tr><td>Turn to Day mode at</td><td>"));
  sendTagInputTime(F("turnToDayModeAt"), 0, upTime);
  sendRawData(F("</td></tr>"));
  sendRawData(F("<tr><td>Turn to Night mode at</td><td>"));
  sendTagInputTime(F("turnToNightModeAt"), 0, downTime);
  sendRawData(F("</td></tr>"));
  sendRawData(F("</table>"));
  sendRawData(F("<input type='submit' value='Save'>"));
  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));
  
  sendRawData(F("<br/>"));
  if (c_isWifiResponseError) return;

  sendRawData(F("<fieldset><legend>Temperature</legend>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post' onSubmit='if (document.getElementById(\"normalTemperatueDayMin\").value >= document.getElementById(\"normalTemperatueDayMax\").value "));
  sendRawData(F("|| document.getElementById(\"normalTemperatueNightMin\").value >= document.getElementById(\"normalTemperatueDayMax\").value) { alert(\"Temperature ranges are incorrect\"); return false;}'>"));
  sendRawData(F("<table>"));
  sendRawData(F("<tr><td>Normal Day temperature</td><td>"));
  sendTagInputNumber(F("normalTemperatueDayMin"), 0, 1, 50, normalTemperatueDayMin);
  sendRawData(F(" .. "));
  sendTagInputNumber(F("normalTemperatueDayMax"), 0, 1, 50, normalTemperatueDayMax);
  sendRawData(F("&deg;C</td></tr>"));
  sendRawData(F("<tr><td>Normal Night temperature</td><td>"));
  sendTagInputNumber(F("normalTemperatueNightMin"), 0, 1, 50, normalTemperatueNightMin);
  sendRawData(F(" .. "));
  sendTagInputNumber(F("normalTemperatueNightMax"), 0, 1, 50, normalTemperatueNightMax);
  sendRawData(F("&deg;C</td></tr>"));
  sendRawData(F("<tr><td>Critical temperature</td><td>"));
  sendTagInputNumber(F("criticalTemperatue"), 0, 1, 50, criticalTemperatue);
  sendRawData(F("&deg;C</td></tr>"));
  sendRawData(F("</table>"));
  sendRawData(F("<input type='submit' value='Save'>"));
  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));
  sendRawData(F("<br/>"));

  sendRawData(F("<fieldset><legend>Logger</legend>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post'>"));
  sendTagCheckbox(F("isStoreLogRecordsEnabled"), F("Enable logger"), GB_StorageHelper.isStoreLogRecordsEnabled());
  sendRawData(F("<br/><input type='submit' value='Save'>"));
  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));
  
  sendRawData(F("<br/>"));
  if (c_isWifiResponseError) return;
  
  boolean isWifiStationMode = GB_StorageHelper.isWifiStationMode();
  sendRawData(F("<fieldset><legend>Wi-Fi</legend>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post'>"));
  sendTagRadioButton(F("isWifiStationMode"), F("Create new Network"), F("0"),!isWifiStationMode);
  sendRawData(F("<div class='description'>Hidden, security WPA2, ip 192.168.0.1</div>"));
  sendTagRadioButton(F("isWifiStationMode"), F("Join existed Network"), F("1"), isWifiStationMode);
  sendRawData(F("<table>"));
  sendRawData(F("<tr><td><label for='wifiSSID'>SSID</label></td><td><input type='text' name='wifiSSID' required value='"));
  sendRawData(GB_StorageHelper.getWifiSSID());
  sendRawData(F("' maxlength='"));
  sendRawData(WIFI_SSID_LENGTH);
  sendRawData(F("'/></td></tr>"));
  sendRawData(F("<tr><td><label for='wifiPass'>Password</label></td><td><input type='password' name='wifiPass' value='"));
  sendRawData(GB_StorageHelper.getWifiPass());
  sendRawData(F("' maxlength='"));
  sendRawData(WIFI_PASS_LENGTH);
  sendRawData(F("'/></td></tr>"));
  sendRawData(F("</table>")); 
  sendRawData(F("<input type='submit' value='Save'><small>and reboot Growbox manually</small>"));
  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));

  sendRawData(F("<br/>"));
  if (c_isWifiResponseError) return;
  
  sendRawData(F("<fieldset><legend>EEPROM</legend>"));  
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_configuration));
  sendRawData(F("' method='post' onSubmit='return confirm(\"Logged records maybe will be reset?\")'>")); 
  sendRawData(F("<table class='grab'>"));
  sendRawData(F("<tr><td colspan='2'>"));
  sendTagCheckbox(F("isEEPROM_AT24C32_Connected"), F("Use external AT24C32 EEPROM"), GB_StorageHelper.isUseExternal_EEPROM_AT24C32());
  sendRawData(F("<div class='description'>Current state [<b>"));
  sendTextRedIfTrue(EEPROM_AT24C32.isPresent() ? F("Connected") : F("Not connected"), GB_StorageHelper.isUseExternal_EEPROM_AT24C32() && !EEPROM_AT24C32.isPresent());
  sendRawData(F("</b>]</div>"));
  sendRawData(F("</td></tr>"));
  sendRawData(F("<tr><td>"));
  sendRawData(F("<input type='submit' value='Save'>"));
  sendRawData(F("</td><td class='align_right'>"));
  sendRawData(F("<a href='"));
  sendRawData(FS(S_url_storage));
  sendRawData(F("'>View EEPROM dumps</a>")); 
  sendRawData(F("</td></tr>"));
  sendRawData(F("</table>")); 
  sendRawData(F("</form>"));
  sendRawData(F("</fieldset>"));

  sendRawData(F("<br/>"));
  if (c_isWifiResponseError) return;
  
  sendRawData(F("<fieldset><legend>Other</legend>"));  
  sendRawData(F("<table style='vertical-align:top; border-spacing:0px;'>"));
  sendRawData(F("<tr><td>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_root));
  sendRawData(F("' method='post' onSubmit='return confirm(\"Reboot Growbox?\")'>"));
  sendRawData(F("<input type='hidden' name='rebootController'/><input type='submit' value='Reboot Growbox'>"));
  sendRawData(F("</form>")); 
  sendRawData(F("</td><td>"));
  sendRawData(F("<small> and update page manually</small>"));
  sendRawData(F("</td></tr><tr><td>"));
  sendRawData(F("<form action='"));
  sendRawData(FS(S_url_root));
  sendRawData(F("' method='post' onSubmit='return confirm(\"Reset Firmware?\")'>"));
  sendRawData(F("<input type='hidden' name='resetFirmware'/><input type='submit' value='Reset Firmware'>"));
  sendRawData(F("</form>"));
  sendRawData(F("</td><td>"));
  sendRawData(F("<small>and update page manually. Default Wi-Fi SSID and pass will be used  </small>"));
  sendRawData(F("</td></tr></table>"));
  sendRawData(F("</fieldset>"));
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
  sendAppendOptionToSelectDynamic(F("typeOfStorageCombobox"), F("r"), F("AT24C32 (external)"), !isArduino);

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

  if (!isArduino && !EEPROM_AT24C32.isPresent()){
     sendRawData(F("<br/>External AT24C32 EEPROM not connected"));
     return;
  }
  
  sendRawData(F("<table class='grab align_center'><tr><th/>"));
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

    boolean result = applyPostParam(url, name, value);

    if (g_useSerialMonitor){
      showWebMessage(F("Execute ["), false);
      Serial.print(name);
      Serial.print('=');
      Serial.print(value);
      Serial.print(F("] - "));
      Serial.println(result ? F("OK") : F("FAIL"));
    }

    index++;
  }
  //if (index > 0){
  //  return url + String('?') + queryStr;
  //}
  //else {
  return url;
  //}
}

boolean WebServerClass::applyPostParam(const String& url, const String& name, const String& value){

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
  else if (StringUtils::flashStringEquals(name, F("resetStoredLog"))){
    GB_StorageHelper.resetStoredLog();
  } 
  else if (StringUtils::flashStringEquals(name, F("isStoreLogRecordsEnabled"))){
    if (value.length() != 1){
      return false;
    }
    GB_StorageHelper.setStoreLogRecordsEnabled(value[0]=='1'); 
  }
  else if (StringUtils::flashStringEquals(name, F("rebootController"))){
    GB_Controller.rebootController();
  } 
  else if (StringUtils::flashStringEquals(name, F("resetFirmware"))){
    GB_StorageHelper.resetFirmware(); 
    GB_Controller.rebootController();
  }

  else if (StringUtils::flashStringEquals(name, F("turnToDayModeAt")) || StringUtils::flashStringEquals(name, F("turnToNightModeAt"))){
    word timeValue = getTimeFromInput(value);
    if (timeValue == 0xFFFF){
      return false;
    }

    if (StringUtils::flashStringEquals(name, F("turnToDayModeAt"))) {
      GB_StorageHelper.setTurnToDayModeTime(timeValue/60, timeValue%60);
    } 
    else if (StringUtils::flashStringEquals(name, F("turnToNightModeAt"))) {
      GB_StorageHelper.setTurnToNightModeTime(timeValue/60, timeValue%60);
    }
    c_isWifiForceUpdateGrowboxState = true;
  } 
  else if (StringUtils::flashStringEquals(name, F("normalTemperatueDayMin")) || 
    StringUtils::flashStringEquals(name, F("normalTemperatueDayMax")) ||  
    StringUtils::flashStringEquals(name, F("normalTemperatueNightMin")) ||
    StringUtils::flashStringEquals(name, F("normalTemperatueNightMax")) ||
    StringUtils::flashStringEquals(name, F("criticalTemperatue"))){
    byte temp = value.toInt();
    if (temp == 0){
      return false;
    }

    if(StringUtils::flashStringEquals(name, F("normalTemperatueDayMin"))){
      GB_StorageHelper.setNormalTemperatueDayMin(temp);
    } 
    else if(StringUtils::flashStringEquals(name, F("normalTemperatueDayMax"))){
      GB_StorageHelper.setNormalTemperatueDayMax(temp);
    } 
    else if(StringUtils::flashStringEquals(name, F("normalTemperatueNightMin"))){
      GB_StorageHelper.setNormalTemperatueNightMin(temp);
    } 
    else if(StringUtils::flashStringEquals(name, F("normalTemperatueNightMax"))){
      GB_StorageHelper.setNormalTemperatueNightMax(temp);
    } 
    else if(StringUtils::flashStringEquals(name, F("criticalTemperatue"))){
      GB_StorageHelper.setCriticalTemperatue(temp);
    }    

    c_isWifiForceUpdateGrowboxState = true;
  }   

  else if (StringUtils::flashStringEquals(name, F("isWetSensorConnected")) || 
    StringUtils::flashStringEquals(name, F("isWaterPumpConnected")) || 
    StringUtils::flashStringEquals(name, F("useWetSensorForWatering")) ||
    StringUtils::flashStringEquals(name, F("skipNextWatering"))){

    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF){
      return false;
    }
    if (value.length() != 1){
      return false;
    }
    boolean boolValue = (value[0]=='1');

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (StringUtils::flashStringEquals(name, F("isWetSensorConnected"))) {
      wsp.boolPreferencies.isWetSensorConnected = boolValue; 
    } 
    else if (StringUtils::flashStringEquals(name, F("isWaterPumpConnected"))) {
      wsp.boolPreferencies.isWaterPumpConnected = boolValue;
    } 
    else if (StringUtils::flashStringEquals(name, F("useWetSensorForWatering"))) {
      wsp.boolPreferencies.useWetSensorForWatering = boolValue;
    }
    else if (StringUtils::flashStringEquals(name, F("skipNextWatering"))) {
      wsp.boolPreferencies.skipNextWatering = boolValue;
    }
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);  

    if (StringUtils::flashStringEquals(name, F("isWaterPumpConnected")) ){
      GB_Watering.updateWateringSchedule();
    }
  }
  else if (StringUtils::flashStringEquals(name, F("inAirValue")) || 
    StringUtils::flashStringEquals(name, F("veryDryValue")) ||     
    StringUtils::flashStringEquals(name, F("dryValue")) || 
    StringUtils::flashStringEquals(name, F("normalValue")) ||
    StringUtils::flashStringEquals(name, F("wetValue")) ||
    StringUtils::flashStringEquals(name, F("veryWetValue")) ){

    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF){
      return false;
    }
    byte intValue = value.toInt();
    if (intValue == 0){
      return false;
    }

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (StringUtils::flashStringEquals(name, F("inAirValue"))){
      wsp.inAirValue = intValue;
    } 
    else if (StringUtils::flashStringEquals(name, F("veryDryValue"))){
      wsp.veryDryValue = intValue;
    } 
    else if (StringUtils::flashStringEquals(name, F("dryValue"))){
      wsp.dryValue = intValue;
    } 
    else if (StringUtils::flashStringEquals(name, F("normalValue"))){
      wsp.normalValue = intValue;
    } 
    else if (StringUtils::flashStringEquals(name, F("wetValue"))){
      wsp.wetValue = intValue;
    } 
    else if (StringUtils::flashStringEquals(name, F("veryWetValue"))){
      wsp.veryWetValue = intValue;
    }
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);  
  } 
  else if (StringUtils::flashStringEquals(name, F("dryWateringDuration")) || 
    StringUtils::flashStringEquals(name, F("veryDryWateringDuration"))){

    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF){
      return false;
    }
    byte intValue = value.toInt();
    if (intValue == 0){
      return false;
    }

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (StringUtils::flashStringEquals(name, F("dryWateringDuration"))){
      wsp.dryWateringDuration = intValue;
    } 
    else if (StringUtils::flashStringEquals(name, F("veryDryWateringDuration"))){
      wsp.veryDryWateringDuration = intValue;
    }
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);  
  } 
  else if (StringUtils::flashStringEquals(name, F("startWateringAt"))){
    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF){
      return false;
    }
    word timeValue = getTimeFromInput(value);
    if (timeValue == 0xFFFF){
      return false;
    }
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    wsp.startWateringAt = timeValue;
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp); 

    GB_Watering.updateWateringSchedule();
  } 
  else if (StringUtils::flashStringEquals(name, F("runDryWateringNow"))){
    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF){
      return false;
    }
    GB_Watering.turnOnWaterPumpManual(wsIndex);
  } 
  else if (StringUtils::flashStringEquals(name, F("clearLastWateringTime"))){
    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF){
      return false;
    }
    GB_Watering.turnOnWaterPumpManual(wsIndex);

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    wsp.lastWateringTimeStamp = 0;
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp); 

    GB_Watering.updateWateringSchedule();
  } 

  else if (StringUtils::flashStringEquals(name, F("setClockTime"))){
    time_t newTimeStamp = strtoul(value.c_str(), NULL, 0);
    if (newTimeStamp == 0){
      return false;
    }
    GB_Controller.setClockTime(newTimeStamp + UPDATE_WEB_SERVER_AVERAGE_PAGE_LOAD_DELAY);

    c_isWifiForceUpdateGrowboxState = true; // Switch to Day/Night mode
  } 
  else if (StringUtils::flashStringEquals(name, F("isEEPROM_AT24C32_Connected"))){
    if (value.length() != 1){
      return false;
    }
    boolean boolValue = (value[0]=='1');   
    GB_StorageHelper.setUseExternal_EEPROM_AT24C32(boolValue);
  } 
  else {
    return false;
  }

  return true;
}

WebServerClass GB_WebServer;


