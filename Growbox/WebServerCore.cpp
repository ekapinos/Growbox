#include "WebServer.h"

#include "RAK410_XBeeWifi.h"
#include "StorageHelper.h"
#include "Controller.h"
#include "StringUtils.h"

void WebServerClass::init() {
  RAK410_XBeeWifi.init();
}

void WebServerClass::update() {
  RAK410_XBeeWifi.update();
}

boolean WebServerClass::handleSerialWiFiEvent() {

  String url, getParams, postParams;

  // HTTP response supplemental   
  RAK410_XBeeWifiClass::RequestType commandType = RAK410_XBeeWifi.handleSerialEvent(c_wifiPortDescriptor, url, getParams, postParams);

  c_isWifiResponseError = false;
  c_isWifiForceUpdateGrowboxState = false;

  switch (commandType) {
    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET:
      httpProcessGet(url, getParams);
      break;

    case RAK410_XBeeWifiClass::RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST:
      httpRedirect(applyPostParams(url, postParams));
      break;

    default:
      break;
  }

  if (g_useSerialMonitor) {
    if (c_isWifiResponseError) {
      showWebMessage(F("Error occurred during sending response"));
    }
  }
  return c_isWifiForceUpdateGrowboxState;
}

boolean WebServerClass::handleSerialMonitorEvent() {

  String input(Serial.readString());

  if (StringUtils::flashStringEndsWith(input, FS(S_CRLF))) {
    input = input.substring(0, input.length() - 2);
  }

  int indexOfQestionChar = input.indexOf('?');
  if (indexOfQestionChar < 0) {
    if (g_useSerialMonitor) {
      Serial.print(F("Wrong serial command ["));
      Serial.print(input);
      Serial.print(F("]. '?' char not found. Syntax: url?param1=value1[&param2=value]"));
    }
    return false;
  }

  String url(input.substring(0, indexOfQestionChar));
  String postParams(input.substring(indexOfQestionChar + 1));

  if (g_useSerialMonitor) {
    Serial.print(F("Receive from [SM] POST ["));
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

void WebServerClass::httpNotFound() {
  RAK410_XBeeWifi.sendFixedSizeData(c_wifiPortDescriptor, F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
  RAK410_XBeeWifi.sendCloseConnection(c_wifiPortDescriptor);
}

// WARNING! RAK 410 became mad when 2 parallel connections comes. Like with Chrome and POST request, when RAK response 303.
// Connection for POST request closed by Chrome (not by RAK). And during this time Chrome creates new parallel connection for GET
// request.
void WebServerClass::httpRedirect(const String &url) {
  //const __FlashStringHelper* header = F("HTTP/1.1 303 See Other\r\nLocation: "); // DO not use it with RAK 410
  const __FlashStringHelper* header = F("HTTP/1.1 200 OK\r\nConnection: close\r\nrefresh: 1; url=");

  RAK410_XBeeWifi.sendFixedSizeFrameStart(c_wifiPortDescriptor, StringUtils::flashStringLength(header) + url.length() + StringUtils::flashStringLength(FS(S_CRLFCRLF)));
  RAK410_XBeeWifi.sendFixedSizeFrameData(header);
  RAK410_XBeeWifi.sendFixedSizeFrameData(url);
  RAK410_XBeeWifi.sendFixedSizeFrameData(FS(S_CRLFCRLF));
  RAK410_XBeeWifi.sendFixedSizeFrameStop();

  RAK410_XBeeWifi.sendCloseConnection(c_wifiPortDescriptor);
}

void WebServerClass::httpPageHeader() {
  RAK410_XBeeWifi.sendFixedSizeData(c_wifiPortDescriptor, F("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"));
  RAK410_XBeeWifi.sendAutoSizeFrameStart(c_wifiPortDescriptor);
}

void WebServerClass::httpPageComplete() {
  RAK410_XBeeWifi.sendAutoSizeFrameStop(c_wifiPortDescriptor);
  RAK410_XBeeWifi.sendCloseConnection(c_wifiPortDescriptor);
}

/////////////////////////////////////////////////////////////////////
//                         HTTP PARAMETERS                         //
/////////////////////////////////////////////////////////////////////

String WebServerClass::encodeHttpString(const String& dirtyValue) {

  String value;
  for (unsigned int i = 0; i < dirtyValue.length(); i++) {
    if (dirtyValue[i] == '+') {
      value += ' ';
    }
    else if (dirtyValue[i] == '%') {
      if (i + 2 < dirtyValue.length()) {
        byte hiCharPart = StringUtils::hexCharToByte(dirtyValue[i + 1]);
        byte loCharPart = StringUtils::hexCharToByte(dirtyValue[i + 2]);
        if (hiCharPart != 0xFF && loCharPart != 0xFF) {
          value += (char)((hiCharPart << 4) + loCharPart);
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

boolean WebServerClass::getHttpParamByIndex(const String& params, const word index, String& name, String& value) {

  word paramsCount = 0;

  int beginIndex = 0;
  int endIndex = params.indexOf('&');
  while (beginIndex < (int)params.length()) {
    if (endIndex == -1) {
      endIndex = params.length();
    }

    if (paramsCount == index) {
      String param = params.substring(beginIndex, endIndex);

      int equalsCharIndex = param.indexOf('=');
      if (equalsCharIndex == -1) {
        name = encodeHttpString(param);
        value = String();
      }
      else {
        name = encodeHttpString(param.substring(0, equalsCharIndex));
        value = encodeHttpString(param.substring(equalsCharIndex + 1));
      }
      return true;
    }

    paramsCount++;

    beginIndex = endIndex + 1;
    endIndex = params.indexOf('&', beginIndex);
  }

  return false;
}

boolean WebServerClass::searchHttpParamByName(const String& params, const __FlashStringHelper* targetName, String& targetValue) {
  String name, value;
  word index = 0;
  while(getHttpParamByIndex(params, index, name, value)) {
    if (StringUtils::flashStringEquals(name, targetName)) {
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

void WebServerClass::rawData(const __FlashStringHelper* data) {
  if (!RAK410_XBeeWifi.sendAutoSizeFrameData(c_wifiPortDescriptor, data)) {
    c_isWifiResponseError = true;
  }
}

void WebServerClass::rawData(const String &data) {
  if (!RAK410_XBeeWifi.sendAutoSizeFrameData(c_wifiPortDescriptor, data)) {
    c_isWifiResponseError = true;
  }
}

void WebServerClass::rawData(float data) {
  String str = StringUtils::floatToString(data);
  rawData(str);
}

void WebServerClass::rawData(time_t data, boolean interpretateAsULong, boolean forceShowZeroTimeStamp) {
  if (interpretateAsULong == true) {
    String str(data);
    rawData(str);
  }
  else {
    if (data == 0 && !forceShowZeroTimeStamp) {
      rawData(F("N/A"));
    }
    else {
      String str = StringUtils::timeStampToString(data);
      rawData(str);
    }
  }
}

void WebServerClass::tagButton(const __FlashStringHelper* url, const __FlashStringHelper* text, boolean isSelected) {
  rawData(F("<input type='button' onclick='document.location=\""));
  rawData(url);
  rawData(F("\"' value='"));
  rawData(text);
  rawData(F("' "));
  if (isSelected) {
    rawData(F("style='text-decoration:underline;' "));
  }
  rawData(F("/>"));
}

void WebServerClass::tagRadioButton(const __FlashStringHelper* name, const __FlashStringHelper* text, const __FlashStringHelper* value, boolean isSelected) {
  rawData(F("<label><input type='radio' name='"));
  rawData(name);
  rawData(F("' value='"));
  rawData(value);
  rawData(F("' "));
  if (isSelected) {
    rawData(F("checked='checked' "));
  }
  rawData(F("/>"));
  rawData(text);
  rawData(F("</label>"));
}

void WebServerClass::tagCheckbox(const __FlashStringHelper* name, const __FlashStringHelper* text, boolean isSelected) {
  rawData(F("<label><input type='checkbox' "));
  if (isSelected) {
    rawData(F("checked='checked' "));
  }
  rawData(F("onchange='document.getElementById(\""));
  rawData(name);
  rawData(F("\").value = (this.checked?1:0)'/>"));
  rawData(text);
  rawData(F("</label>"));
  rawData(F("<input type='hidden' name='"));
  rawData(name);
  rawData(F("' id='"));
  rawData(name);
  rawData(F("' value='"));
  rawData(isSelected?'1':'0');
  rawData(F("'>"));

}

void WebServerClass::tagInputNumber(const __FlashStringHelper* name, const __FlashStringHelper* text, long minValue, long maxValue, long value) {
  if (text != 0) {
    rawData(F("<label>"));
    rawData(text);
  }
  rawData(F("<input type='number' required='required' name='"));
  rawData(name);
  rawData(F("' id='"));
  rawData(name);
  rawData(F("' min='"));
  rawData(minValue);
  rawData(F("' max='"));
  rawData(maxValue);
  rawData(F("' value='"));
  rawData(value);
  rawData(F("'/>"));
  if (text != 0) {
    rawData(F("</label>"));
  }
}

void WebServerClass::tagInputTime(const __FlashStringHelper* name, const __FlashStringHelper* text, word value, const __FlashStringHelper* onChange) {
  if (text != NULL) {
    rawData(F("<label>"));
    rawData(text);
  }
  rawData(F("<input type='time' required='required' step='60' name='"));
  rawData(name);
  rawData(F("' id='"));
  rawData(name);
  rawData(F("' value='"));
  rawData(StringUtils::wordTimeToString(value));
  if (onChange != NULL){
    rawData(F("' onchange='"));
    rawData(onChange);
  }
  rawData(F("'/>"));
  if (text != NULL) {
    rawData(F("</label>"));
  }
}

word WebServerClass::getTimeFromInput(const String& value) {
  if (value.length() != 5) {
    return 0xFFFF;
  }

  if (value[2] != ':') {
    return 0xFFFF;
  }

  byte valueHour = (value[0] - '0') * 10 + (value[1] - '0');
  byte valueMinute = (value[3] - '0') * 10 + (value[4] - '0');
  return valueHour * 60 + valueMinute;
}

void WebServerClass::tagOption(const String& value, const String& text, boolean isSelected, boolean isDisabled) {
  rawData(F("<option"));
  if (value.length() != 0) {
    rawData(F(" value='"));
    rawData(value);
    rawData('\'');
  }
  if (isDisabled) {
    rawData(F(" disabled='disabled'"));
  }
  if (isSelected) {
    rawData(F(" selected='selected'"));
  }
  rawData('>');
  rawData(text);
  rawData(F("</option>"));
}
void WebServerClass::tagOption(const __FlashStringHelper* value, const __FlashStringHelper* text, boolean isSelected, boolean isDisabled) {
  tagOption(StringUtils::flashStringLoad(value), StringUtils::flashStringLoad(text), isSelected, isDisabled);
}

void WebServerClass::appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const String& value, const String& text, boolean isSelected) {
  rawData(F("<script type='text/javascript'>"));
  rawData(F("var opt = document.createElement('option');"));
  if (value.length() != 0) {
    rawData(F("opt.value = '"));
    rawData(value);
    rawData(F("';"));
  }
  rawData(F("opt.text = '"));    //opt.value = i;
  rawData(text);
  rawData(F("';"));
  if (isSelected) {
    rawData(F("opt.selected = true;"));
    rawData(F("opt.style.fontWeight = 'bold';"));
  }
  rawData(F("document.getElementById('"));
  rawData(selectId);
  rawData(F("').appendChild(opt);"));
  rawData(F("</script>"));
}

void WebServerClass::appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const String& text, boolean isSelected) {
  appendOptionToSelectDynamic(selectId, StringUtils::flashStringLoad(value), text, isSelected);
}

void WebServerClass::appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const __FlashStringHelper* text, boolean isSelected) {
  appendOptionToSelectDynamic(selectId, StringUtils::flashStringLoad(value), StringUtils::flashStringLoad(text), isSelected);
}

void WebServerClass::growboxClockJavaScript(const __FlashStringHelper* growboxTimeStampId, const __FlashStringHelper* browserTimeStampId, const __FlashStringHelper* diffTimeStampId, const __FlashStringHelper* setClockTimeHiddenInputId) {

  rawData(F("<script type='text/javascript'>"));
  rawData(F("var g_timeFormat={year:'numeric',month:'2-digit',day:'2-digit',hour:'2-digit',minute:'2-digit',second:'2-digit'};"));
  rawData(F("var g_gbts=new Date((")); //growbox timestamp
  rawData(now() + WEB_SERVER_AVERAGE_PAGE_LOAD_TIME_SEC, true);
  rawData(F("+new Date().getTimezoneOffset()*60)*1000"));
  rawData(F(");"));

  rawData(F("var g_TSDiff=new Date().getTime()-g_gbts;"));
  if (diffTimeStampId != NULL) {
    rawData(F("var diffStr='';"));
    rawData(F("var absTSDiffSec=Math.abs(Math.floor(g_TSDiff/1000));"));
    rawData(F("if (absTSDiffSec>0){"));
    {
      rawData(F("var diffSeconds=absTSDiffSec%60;"));
      rawData(F("var diffMinutes=Math.floor(absTSDiffSec/60)%60;"));
      rawData(F("var diffHours=Math.floor(absTSDiffSec/60/60);"));
      rawData(F("if(diffHours>365*24){"));
      {
        rawData(F("diffStr='over year';"));
      }
      rawData(F("}else if (diffHours>30*24){"));
      {
        rawData(F("diffStr='over month';"));
      }
      rawData(F("}else if (diffHours>7*24){"));
      {
        rawData(F("diffStr='over week';"));
      }
      rawData(F("}else if (diffHours>24){"));
      {
        rawData(F("diffStr='over day';"));
      }
      rawData(F("}else if (diffHours>0){"));
      {
        rawData(F("diffStr=diffHours+' h '+diffMinutes+' m '+diffSeconds+' s';"));
      }
      rawData(F("}else if (diffMinutes > 0){"));
      {
        rawData(F("diffStr=diffMinutes+' min '+diffSeconds+' sec';"));
      }
      rawData(F("}else if (diffSeconds > 0){"));
      {
        rawData(F("diffStr=diffSeconds+' sec';"));
      }
      rawData(F("}"));
      rawData(F("diffStr='Out of sync '+diffStr+' with browser time';"));
    }
    rawData(F("}else{"));
    {
      rawData(F("diffStr='Synced with browser time';"));
    }
    rawData(F("}"));
    rawData(F("var g_diffTimeStamp=document.getElementById('"));
    rawData(diffTimeStampId);
    rawData(F("');"));
    if (GB_StorageHelper.isAutoCalculatedClockTimeUsed()) {
      rawData(F("diffStr='Auto calculated. '+diffStr;"));
      rawData(F("g_diffTimeStamp.style.color='red';"));
    }
    else {
      rawData(F("if (absTSDiffSec>60){g_diffTimeStamp.style.color='red';}"));
    }
    rawData(F("g_diffTimeStamp.innerHTML=diffStr;"));
  }

  rawData(F("function g_uts(){"));{ // update time stamps
    rawData(F("var bts=new Date();")); // browser time stamp
    rawData(F("g_gbts.setTime(bts.getTime()-g_TSDiff);"));
    if (growboxTimeStampId != NULL) {
      rawData(F("document.getElementById('"));
      rawData(growboxTimeStampId);
      rawData(F("').innerHTML=g_gbts.toLocaleString('uk',g_timeFormat);"));
    }
    if (browserTimeStampId != NULL) {
      rawData(F("document.getElementById('"));
      rawData(browserTimeStampId);
      rawData(F("').innerHTML=bts.toLocaleString('uk',g_timeFormat);"));
    }
    rawData(F("setTimeout(function () {g_uts(); }, 1000);"));
  }
  rawData(F("};"));

  rawData(F("g_uts();"));

  if (setClockTimeHiddenInputId != NULL){
    rawData(F("var g_checkBeforeSetClockTime=function(){"));{
      rawData(F("if(!confirm(\"Syncronize Growbox time with browser time?\")) {"));{
        rawData(F("return false;"));
      }
      rawData(F("}"));
      rawData(F("document.getElementById(\""));
      rawData(setClockTimeHiddenInputId);
      rawData(F("\").value=Math.floor(new Date().getTime()/1000-new Date().getTimezoneOffset()*60);"));
      rawData(F("return true;"));
    }
    rawData(F("}"));
  }

  rawData(F("</script>"));
}

void WebServerClass::spanTag_RedIfTrue(const __FlashStringHelper* text, boolean isRed) {
  if (isRed) {
    rawData(F("<span class='red'>"));
  }
  else {
    rawData(F("<span>"));
  }
  rawData(text);
  rawData(F("</span>"));
}

void WebServerClass::updateDayNightPeriodJavaScript() {
  rawData(F("<script type='text/javascript'>"));
  rawData(F("function g_wordTimeToString(time){"));{
    rawData(F("var hour=Math.floor(time/60);"));
    rawData(F("var minute=time%60;"));
    rawData(F("return (hour<10?'0':'')+hour+':'+(minute<10?'0':'')+minute;"));
  }
  rawData(F("}"));
  rawData(F("function g_updDayNightPeriod(){"));
  {
    rawData(F("var upTime=new Date('01.01.2000 '+document.getElementById('turnToDayModeAt').value+':00');"));
    rawData(F("var downTime=new Date('01.01.2000 '+document.getElementById('turnToNightModeAt').value+':00');"));
    rawData(F("var delta=Math.floor((downTime-upTime)/1000/60);")); // in minutes
    rawData(F("var dayPeriod=(delta>0?delta:24*60+delta);"));
    rawData(F("document.getElementById(\"dayNightPeriod\").innerHTML=g_wordTimeToString(dayPeriod)+'/'+g_wordTimeToString(24*60-dayPeriod);"));
  }
  rawData(F("}"));
  rawData(F("g_updDayNightPeriod();"));
  rawData(F("</script>"));
}

// Returns 24:00 if dates equal
word WebServerClass::getWordTimePeriodinDay(word start, word stop){
  if (stop > start){
    return stop - start;
  } else {
    return 24 * 60 - (start - stop);
  }
}

void WebServerClass::printTemperatue(float t) {
  if (isnan(t)) {
    rawData(F("N/A"));
  }
  else {
    rawData(t);
    rawData(F("&deg;C"));
  }
}

void WebServerClass::printTemperatueRange(float t1, float t2) {
  if (isnan(t1)) {
    rawData(F("N/A"));
  }
  else {
    rawData(t1);
  }
  rawData(F(".."));
  printTemperatue(t2);
}

void WebServerClass::printFanSpeed(byte fanSpeedValue) {
  boolean isOn;
  byte speed, numerator, denominator;
  GB_Controller.unpackFanSpeedValue(fanSpeedValue, isOn, speed, numerator, denominator);

  if (isOn) {
    rawData(speed == FAN_SPEED_HIGH ? F("high speed") : F("low speed"));
    if (numerator != 0 || denominator != 0){
      rawData(F(", "));
      rawData(numerator * UPDATE_GROWBOX_STATE_DELAY_MINUTES);
      rawData('/');
      rawData(denominator * UPDATE_GROWBOX_STATE_DELAY_MINUTES);
      rawData(F(" min"));
    }
  }
  else {
    rawData(F("off"));
  }
}

WebServerClass GB_WebServer;

