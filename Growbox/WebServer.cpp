#include "WebServer.h"

#include <MemoryFree.h>
#include "Controller.h"
#include "StorageHelper.h" 
#include "Thermometer.h" 
#include "Watering.h"
#include "RAK410_XBeeWifi.h" 
#include "EEPROM_AT24C32.h" 

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

void WebServerClass::tagInputTime(const __FlashStringHelper* name, const __FlashStringHelper* text, word value) {
  if (text != 0) {
    rawData(F("<label>"));
    rawData(text);
  }
  rawData(F("<input type='time' required='required' step='60' name='"));
  rawData(name);
  rawData(F("' id='"));
  rawData(name);
  rawData(F("' value='"));
  rawData(StringUtils::wordTimeToString(value));
  rawData(F("'/>"));
  if (text != 0) {
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

void WebServerClass::growboxClockJavaScript(const __FlashStringHelper* growboxTimeStampId, const __FlashStringHelper* browserTimeStampId, const __FlashStringHelper* diffTimeStampId) {

  rawData(F("<script type='text/javascript'>"));
  rawData(F("var g_timeFormat = {year:'numeric',month:'2-digit',day:'2-digit',hour:'2-digit',minute:'2-digit',second:'2-digit'};"));
  rawData(F("var g_GBTS = new Date((")); //growbox timestamp
  rawData(now() + UPDATE_WEB_SERVER_AVERAGE_PAGE_LOAD_DELAY, true);
  rawData(F(" + new Date().getTimezoneOffset()*60) * 1000"));
  //  tmElements_t nowTmElements; 
  //  breakTime(now(), nowTmElements);
  //  rawData(tmYearToCalendar(nowTmElements.Year));
  //  rawData(',');
  //  rawData(nowTmElements.Month-1);
  //  rawData(',');
  //  rawData(nowTmElements.Day);
  //  rawData(',');
  //  rawData(nowTmElements.Hour);
  //  rawData(',');
  //  rawData(nowTmElements.Minute);
  //  rawData(',');
  //  rawData(nowTmElements.Second);
  rawData(F(");"));
  rawData(F("var g_TSDiff=new Date().getTime()-g_GBTS;"));
  if (diffTimeStampId != NULL) {
    rawData(F("var resultDiffString = '';"));
    rawData(F("var absTSDiffSec = Math.abs(Math.floor(g_TSDiff/1000));"));
    rawData(F("if (absTSDiffSec>0){"));
    {
      rawData(F("var diffSeconds = absTSDiffSec % 60;"));
      rawData(F("var diffMinutes = Math.floor(absTSDiffSec/60) % 60;"));
      rawData(F("var diffHours = Math.floor(absTSDiffSec/60/60);"));
      rawData(F("if (diffHours>365*24) {"));
      {
        rawData(F("resultDiffString='over year';"));
      }
      rawData(F("} else if (diffHours>30*24) {"));
      {
        rawData(F("resultDiffString='over month';"));
      }
      rawData(F("} else if (diffHours>7*24) {"));
      {
        rawData(F("resultDiffString='over week';"));
      }
      rawData(F("} else if (diffHours>24) {"));
      {
        rawData(F("resultDiffString='over day';"));
      }
      rawData(F("} else if (diffHours>0) {"));
      {
        rawData(F("resultDiffString=diffHours+' h '+diffMinutes+' m '+diffSeconds+' s';"));
      }
      rawData(F("} else if (diffMinutes > 0) {"));
      {
        rawData(F("resultDiffString=diffMinutes+' min '+diffSeconds+' sec';"));
      }
      rawData(F("} else if (diffSeconds > 0) {"));
      {
        rawData(F("resultDiffString=diffSeconds+' sec';"));
      }
      rawData(F("}"));
      rawData(F("resultDiffString='Out of sync '+resultDiffString+' with browser time';"));
    }
    rawData(F("} else {"));
    {
      rawData(F("resultDiffString='Synced with browser time';"));
    }
    rawData(F("}"));
    rawData(F("var g_diffSpanDesc=document.getElementById('"));
    rawData(diffTimeStampId);
    rawData(F("');"));
    if (GB_StorageHelper.isAutoCalculatedClockTimeUsed()) {
      rawData(F("resultDiffString='Auto calculated. '+resultDiffString;"));
      rawData(F("g_diffSpanDesc.style.color='red';"));
    }
    else {
      rawData(F("if (absTSDiffSec>60) { g_diffSpanDesc.style.color='red'; }"));
    }
    rawData(F("g_diffSpanDesc.innerHTML=resultDiffString;"));
  }

  rawData(F("function updateTimeStamps() {"));
  rawData(F("    var browserTimeStamp = new Date();"));
  rawData(F("    g_GBTS.setTime(browserTimeStamp.getTime() - g_TSDiff);"));
  if (growboxTimeStampId != NULL) {
    rawData(F("document.getElementById('"));
    rawData(growboxTimeStampId);
    rawData(F("').innerHTML = g_GBTS.toLocaleString('uk', g_timeFormat);"));
  }
  if (browserTimeStampId != NULL) {
    rawData(F("document.getElementById('"));
    rawData(browserTimeStampId);
    rawData(F("').innerHTML = browserTimeStamp.toLocaleString('uk', g_timeFormat);"));
  }
  rawData(F("    setTimeout(function () {updateTimeStamps(); }, 1000);"));
  rawData(F("};"));
  rawData(F("updateTimeStamps();"));

  rawData(F("var g_checkBeforeSetClockTime = function () {"));
  rawData(F("  if(!confirm(\"Syncronize Growbox time with browser time?\")) {"));
  rawData(F("    return false;"));
  rawData(F("  }"));
  rawData(F("  document.getElementById(\"setClockTimeInput\").value = Math.floor(new Date().getTime()/1000 - new Date().getTimezoneOffset()*60);"));
  rawData(F("  return true;"));
  rawData(F("}"));

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

/////////////////////////////////////////////////////////////////////
//                        COMMON FOR ALL PAGES                     //
/////////////////////////////////////////////////////////////////////

void WebServerClass::httpProcessGet(const String& url, const String& getParams) {

  byte wsIndex = getWateringIndexFromUrl(url); // FF ig not watering system

  boolean isRootPage = StringUtils::flashStringEquals(url, FS(S_url_root));
  boolean isLogPage = StringUtils::flashStringEquals(url, FS(S_url_log));

  boolean isGeneralPage = StringUtils::flashStringEquals(url, FS(S_url_general));
  boolean isWateringPage = (wsIndex != 0xFF);
  boolean isHardwarePage = StringUtils::flashStringEquals(url, FS(S_url_hardware));
  boolean isDumpInternal = StringUtils::flashStringEquals(url, FS(S_url_dump_internal));
  boolean isDumpAT24C32 = StringUtils::flashStringEquals(url, FS(S_url_dump_AT24C32));
  boolean isOtherPage = StringUtils::flashStringEquals(url, FS(S_url_other));

  boolean isConfigurationPage = (isGeneralPage || isWateringPage || isHardwarePage || isDumpInternal || isDumpAT24C32 || isOtherPage);

  boolean isValidPage = (isRootPage || isLogPage || isConfigurationPage);
  if (!isValidPage) {
    httpNotFound();
    return;
  }

  httpPageHeader();

  if (isRootPage || isWateringPage) {
#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME
    if (g_useSerialMonitor) {
      Serial.println(); // We cut log stream to show wet status in new line
    }
#endif
    GB_Watering.preUpdateWetSatus();
  }

  rawData(F("<!DOCTYPE html>")); // HTML 5
  rawData(F("<html><head>"));
  rawData(F("  <title>Growbox</title>"));
  rawData(F("  <meta name='viewport' content='width=device-width, initial-scale=1'/>"));
  rawData(F("<style type='text/css'>"));
  rawData(F("  body {font-family:Arial; max-width:600px; }")); // 350px 
  rawData(F("  form {margin: 0px;}"));
  rawData(F("  dt {font-weight: bold; margin-top:5px;}"));

  rawData(F("  .red {color: red;}"));
  rawData(F("  .grab {border-spacing:5px; width:100%; }"));
  rawData(F("  .align_right {float:right; text-align:right;}"));
  rawData(F("  .align_center {float:center; text-align:center;}"));
  rawData(F("  .description {font-size:small; margin-left:20px; margin-bottom:5px;}"));
  rawData(F("</style>"));
  rawData(F("</head>"));

  rawData(F("<body>"));
  rawData(F("<h1>Growbox</h1>"));
  rawData(F("<form>"));   // HTML Validator warning
  tagButton(FS(S_url_root), F("Status"), isRootPage);
  tagButton(FS(S_url_log), F("Daily log"), isLogPage);
  rawData(F("<select id='configPageSelect' onchange='g_onChangeConfigPageSelect();' "));
  if (isConfigurationPage) {
    rawData(F(" style='text-decoration: underline;'"));
  }
  rawData('>');
  tagOption(F(""), F("Select Configuration page"), !isConfigurationPage, true);
  tagOption(FS(S_url_general), F("General"), isGeneralPage);
  for (byte i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++) {
    String url = StringUtils::flashStringLoad(FS(S_url_watering));
    if (i > 0) {
      url += '/';
      url += (i + 1);
    }
    String description = StringUtils::flashStringLoad(F("Watering system #"));
    description += (i + 1);
    tagOption(url, description, (wsIndex == i));
  }

  tagOption(FS(S_url_hardware), F("Hardware"), isHardwarePage);
  tagOption(FS(S_url_other), F("Other"), isOtherPage);
  if (isDumpInternal || isDumpAT24C32) {
    tagOption(FS(S_url_dump_internal), F("Other: Arduino dump"), isDumpInternal);
    if (EEPROM_AT24C32.isPresent()) {
      tagOption(FS(S_url_dump_AT24C32), F("Other: AT24C32 dump"), isDumpAT24C32);
    }
  }
  rawData(F("</select>"));
  rawData(F("</form>"));

  rawData(F("<script type='text/javascript'>"));
  rawData(F("var g_configPageSelect = document.getElementById('configPageSelect');"));
  rawData(F("var g_configPageSelectedDefault = g_configPageSelect.value;"));
  rawData(F("function g_onChangeConfigPageSelect(event) {"));
  rawData(F("  var newValue = g_configPageSelect.value;"));
  rawData(F("  g_configPageSelect.value = g_configPageSelectedDefault;"));
  rawData(F("  location=newValue;"));
  rawData(F("}"));
  rawData(F("</script>"));

  rawData(F("<hr/>"));

  if (isRootPage) {
    sendStatusPage();
  }
  else if (isLogPage) {
    sendLogPage(getParams);
  }
  else if (isGeneralPage) {
    sendGeneralPage(getParams);
  }
  else if (isWateringPage) {
    sendWateringPage(url, wsIndex);
  }
  else if (isHardwarePage) {
    sendHardwarePage(getParams);
  }
  else if (isDumpInternal || isDumpAT24C32) {
    sendStorageDumpPage(getParams, isDumpInternal);
  }
  else if (isOtherPage) {
    sendOtherPage(getParams);
  }

  rawData(F("</body></html>"));

  httpPageComplete();

}

/////////////////////////////////////////////////////////////////////
//                          STATUS PAGE                            //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendStatusPage() {

  rawData(F("<dl>"));

  rawData(F("<dt>General</dt>"));

  if (GB_Controller.isBreezeFatalError()) {
    rawData(F("<dd>"));
    spanTag_RedIfTrue(F("No breeze"), true);
    rawData(F("</dd>"));
  }

  rawData(F("<dd>"));
  rawData(g_isDayInGrowbox ? F("Day") : F("Night"));
  rawData(F(" mode</dd>"));

  if (GB_Controller.isUseLight()) {
    rawData(F("<dd>Light: "));
    rawData(GB_Controller.isLightTurnedOn() ? F("Enabled") : F("Disabled"));
    rawData(F("</dd>"));
  }

  if (GB_Controller.isUseFan()) {
    rawData(F("<dd>Fan: "));
    boolean isFanEnabled = GB_Controller.isFanTurnedOn();
    rawData(isFanEnabled ? F("Enabled, ") : F("Disabled"));
    if (isFanEnabled) {
      rawData(
          (GB_Controller.getFanSpeed() == FAN_SPEED_MIN) ? F("min speed") : F("max speed"));
    }
    rawData(F("</dd>"));
  }

  if (GB_Controller.isUseRTC()) {
    if (!GB_Controller.isRTCPresent()) {
      rawData(F("<dd>Real-time clock: "));
      spanTag_RedIfTrue(F("Not connected"), true);
      rawData(F("</dd>"));
    }
    else if (GB_Controller.isClockNeedsSync() || GB_Controller.isClockNotSet()) {
      rawData(F("<dd>Clock: "));
      if (GB_Controller.isClockNotSet()) {
        spanTag_RedIfTrue(F("Not set"), true);
      }
      if (GB_Controller.isClockNeedsSync()) {
        spanTag_RedIfTrue(F("Needs sync"), true);
      }
      rawData(F("</dd>"));
    }
  }

  rawData(F("<dd>Startup: "));
  rawData(GB_StorageHelper.getStartupTimeStamp(), false, true);
  rawData(F("</dd>"));

  rawData(F("<dd>Date &amp; time: "));
  rawData(F("<span id='growboxTimeStampId'></span>"));
  rawData(F("<br/><small><span id='diffTimeStampId'></span></small>"));
  growboxClockJavaScript(F("growboxTimeStampId"), NULL, F("diffTimeStampId"));
  rawData(F("<form action='"));
  rawData(FS(S_url_root));
  rawData(F("' method='post' onSubmit='return g_checkBeforeSetClockTime()'>"));
  rawData(F("<input type='hidden' name='setClockTime' id='setClockTimeInput'>"));
  rawData(F("<input type='submit' value='Sync'>"));
  rawData(F("</form>"));
  rawData(F("</dd>"));

  if (c_isWifiResponseError)
    return;

  rawData(F("<dt>Logger</dt>"));
  if (GB_StorageHelper.isUseExternal_EEPROM_AT24C32() && !EEPROM_AT24C32.isPresent()) {
    rawData(F("<dd>External AT24C32 EEPROM: "));
    spanTag_RedIfTrue(F("Not connected"), true);
    rawData(F("</dd>"));
  }
  if (!GB_StorageHelper.isStoreLogRecordsEnabled()) {
    rawData(F("<dd>"));
    spanTag_RedIfTrue(F("Disabled"), true);
    rawData(F("</dd>"));
  }
  rawData(F("<dd>Stored records: "));
  if (GB_StorageHelper.isLogOverflow()) {
    rawData(F("<span class='red'>"));
  }
  rawData(GB_StorageHelper.getLogRecordsCount());
  rawData('/');
  rawData(GB_StorageHelper.getLogRecordsCapacity());
  if (GB_StorageHelper.isLogOverflow()) {
    rawData(F("</span>"));
  }
  rawData(F("</dd>"));

  if (c_isWifiResponseError)
    return;

  if (GB_StorageHelper.isUseThermometer()) {
    float lastTemperature, statisticsTemperature;
    int statisticsCount;
    GB_Thermometer.getStatistics(lastTemperature, statisticsTemperature, statisticsCount);

    rawData(F("<dt>Thermometer</dt>"));
    if (!GB_Thermometer.isPresent()) {
      spanTag_RedIfTrue(F("<dd>Not connected</dd>"), true);
    }
    rawData(F("<dd>Last check: "));
    if (isnan(lastTemperature)) {
      rawData(F("N/A"));
    }
    else {
      rawData(lastTemperature);
      rawData(F(" &deg;C"));
    }
    rawData(F("</dd>"));
    rawData(F("<dd>Forecast: "));
    if (isnan(statisticsTemperature)) {
      rawData(F("N/A"));
    }
    else {
      rawData(statisticsTemperature);
      rawData(F(" &deg;C"));
    }
    rawData(F(" ("));
    rawData(statisticsCount);
    rawData(F(" measurement"));
    if (statisticsCount > 1) {
      rawData('s');
    }
    rawData(F(")</dd>"));
  }
  if (c_isWifiResponseError)
    return;

#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME
  if (g_useSerialMonitor) {
    Serial.println(); // We cut log stream to show wet status in new line
  }
#endif
  GB_Watering.updateWetSatus();
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++) {

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

    if (!wsp.boolPreferencies.isWetSensorConnected && !wsp.boolPreferencies.isWaterPumpConnected) {
      continue;
    }

    rawData(F("<dt>Watering system #"));
    rawData(wsIndex + 1);
    rawData(F("</dt>"));

    if (wsp.boolPreferencies.isWetSensorConnected) {
      rawData(F("<dd>Wet sensor: "));
      WateringEvent* currentStatus = GB_Watering.getCurrentWetSensorStatus(wsIndex);
      spanTag_RedIfTrue(currentStatus->shortDescription, currentStatus != &WATERING_EVENT_WET_SENSOR_NORMAL);

      byte value = GB_Watering.getCurrentWetSensorValue(wsIndex);
      if (!GB_Watering.isWetSensorValueReserved(value)){
        rawData(F(", value ["));
        rawData(value);
        rawData(F("]"));
      }
      rawData(F("</dd>"));
    }

    if (wsp.boolPreferencies.isWaterPumpConnected) {
      rawData(F("<dd>Last watering: "));
      rawData(GB_Watering.getLastWateringTimeStampByIndex(wsIndex));
      rawData(F("</dd>"));

      rawData(F("<dd>Next watering: "));
      rawData(GB_Watering.getNextWateringTimeStampByIndex(wsIndex));
      rawData(F("</dd>"));
    }
  }

  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  rawData(F("<dt>Configuration</dt>"));
  rawData(F("<dd>Day mode at: "));
  rawData(StringUtils::wordTimeToString(upTime));
  rawData(F("</dd><dd>Night mode at: "));
  rawData(StringUtils::wordTimeToString(downTime));
  rawData(F("</dd>"));

  if (GB_StorageHelper.isUseThermometer()) {
    byte normalTemperatueDayMin, normalTemperatueDayMax,
        normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue;
    GB_StorageHelper.getTemperatureParameters(normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue);

    rawData(F("<dd>Normal Day temperature: "));
    rawData(normalTemperatueDayMin);
    rawData(F(".."));
    rawData(normalTemperatueDayMax);
    rawData(F(" &deg;C</dd><dd>Normal Night temperature: "));
    rawData(normalTemperatueNightMin);
    rawData(F(".."));
    rawData(normalTemperatueNightMax);
    rawData(F(" &deg;C</dd><dd>Critical temperature: "));
    rawData(criticalTemperatue);
    rawData(F(" &deg;C</dd>"));
  }

  rawData(F("<dt>Other</dt>"));
  rawData(F("<dd>Free memory: "));
  rawData(freeMemory());
  rawData(F(" bytes</dd>"));
  rawData(F("<dd>First startup: "));
  rawData(GB_StorageHelper.getFirstStartupTimeStamp(), false, true);
  rawData(F("</dd>"));

  rawData(F("</dl>"));
}

/////////////////////////////////////////////////////////////////////
//                             LOG PAGE                            //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendLogPage(const String& getParams) {

  LogRecord logRecord;

  tmElements_t targetTm;
  tmElements_t currentTm;
  tmElements_t previousTm;

  String paramValue;

  // fill targetTm
  boolean isTargetDayFound = false;
  if (searchHttpParamByName(getParams, F("date"), paramValue)) {

    if (paramValue.length() == 10) {
      byte dayInt = paramValue.substring(0, 2).toInt();
      byte monthInt = paramValue.substring(3, 5).toInt();
      word yearInt = paramValue.substring(6, 10).toInt();

      if (dayInt != 0 && monthInt != 0 && yearInt != 0) {
        targetTm.Day = dayInt;
        targetTm.Month = monthInt;
        targetTm.Year = CalendarYrToTm(yearInt);
        isTargetDayFound = true;
      }
    }
  }
  if (!isTargetDayFound) {
    breakTime(now(), targetTm);
  }
  String targetDateAsString = StringUtils::timeStampToString(makeTime(targetTm), true, false);

  boolean printEvents, printWateringEvents, printErrors, printTemperature;
  printEvents = printWateringEvents = printErrors = printTemperature = true;

  if (searchHttpParamByName(getParams, F("type"), paramValue)) {
    if (StringUtils::flashStringEquals(paramValue, F("events"))) {
      printEvents = true;
      printWateringEvents = false;
      printErrors = false;
      printTemperature = false;
    }
    if (StringUtils::flashStringEquals(paramValue, F("wateringevents"))) {
      printEvents = false;
      printWateringEvents = true;
      printErrors = false;
      printTemperature = false;
    }
    else if (StringUtils::flashStringEquals(paramValue, F("errors"))) {
      printEvents = false;
      printWateringEvents = false;
      printErrors = true;
      printTemperature = false;
    }
    else if (StringUtils::flashStringEquals(paramValue, F("temperature"))) {
      printEvents = false;
      printWateringEvents = false;
      printErrors = false;
      printTemperature = true;
    }
    else {
      printEvents = true;
      printWateringEvents = true;
      printErrors = true;
      printTemperature = true;
    }
  }

  if (c_isWifiResponseError)
    return;

  // fill previousTm
  previousTm.Day = previousTm.Month = previousTm.Year = 0; //remove compiller warning

  rawData(F("<form action='"));
  rawData(FS(S_url_log));
  rawData(F("' method='get'>"));

  rawData(F("<select id='typeCombobox' name='type'>"));
  tagOption(F("all"), F("All records"), printEvents && printWateringEvents && printErrors && printTemperature);
  tagOption(F("events"), F("Events only"), printEvents && !printWateringEvents && !printErrors && !printTemperature);
  tagOption(F("wateringevents"), F("Watering Events only"), !printEvents && printWateringEvents && !printErrors && !printTemperature);
  tagOption(F("errors"), F("Errors only"), !printEvents && !printWateringEvents && printErrors && !printTemperature);
  tagOption(F("temperature"), F("Temperature only"), !printEvents && !printWateringEvents && !printErrors && printTemperature);
  rawData(F("</select>"));

  rawData(F("<select id='dateCombobox' name='date'></select>"));
  rawData(F("<input type='submit' value='Show'/>"));
  rawData(F("</form>"));

  boolean isTableTagPrinted = false;
  word index;
  for (index = 0; index < GB_StorageHelper.getLogRecordsCount(); index++) {

    if (c_isWifiResponseError)
      return;

    logRecord = GB_StorageHelper.getLogRecordByIndex(index);
    breakTime(logRecord.timeStamp, currentTm);

    boolean isDaySwitch = !(currentTm.Day == previousTm.Day && currentTm.Month == previousTm.Month && currentTm.Year == previousTm.Year);
    if (isDaySwitch) {
      previousTm = currentTm;
    }
    boolean isTargetDay = (currentTm.Day == targetTm.Day && currentTm.Month == targetTm.Month && currentTm.Year == targetTm.Year);
    String currentDateAsString = StringUtils::timeStampToString(logRecord.timeStamp, true, false);

    if (isDaySwitch) {
      appendOptionToSelectDynamic(F("dateCombobox"), F(""), currentDateAsString, isTargetDay);
    }

    if (!isTargetDay) {
      continue;
    }

    boolean isEvent = GB_Logger.isEvent(logRecord);
    boolean isWateringEvent = GB_Logger.isWateringEvent(logRecord);
    boolean isError = GB_Logger.isError(logRecord);
    boolean isTemperature = GB_Logger.isTemperature(logRecord);

    if (!printEvents && isEvent) {
      continue;
    }
    if (!printWateringEvents && isWateringEvent) {
      continue;
    }
    if (!printErrors && isError) {
      continue;
    }
    if (!printTemperature && isTemperature) {
      continue;
    }

    if (!isTableTagPrinted) {
      isTableTagPrinted = true;
      rawData(F("<table class='grab align_center'>"));
      rawData(F("<tr><th>#</th><th>Time</th><th>Description</th></tr>"));
    }
    rawData(F("<tr"));
    if (isError || logRecord.isEmpty()) {
      rawData(F(" class='red'"));
    }
    else if (isEvent && (logRecord.data == EVENT_FIRST_START_UP.index || logRecord.data == EVENT_RESTART.index)) { // TODO create check method in Logger.h
      rawData(F(" style='font-weight:bold;'"));
    }
    rawData(F("><td>"));
    rawData(index + 1);
    rawData(F("</td><td>"));
    rawData(StringUtils::timeStampToString(logRecord.timeStamp, false, true));
    rawData(F("</td><td style='text-align:left;'>"));
    rawData(GB_Logger.getLogRecordDescription(logRecord));
    rawData(GB_Logger.getLogRecordDescriptionSuffix(logRecord));
    rawData(F("</td></tr>")); // bug with linker was here https://github.com/arduino/Arduino/issues/1071#issuecomment-19832135
  }
  if (isTableTagPrinted) {
    rawData(F("</table>"));
  }
  else {
    appendOptionToSelectDynamic(F("dateCombobox"), F(""), targetDateAsString, true); // TODO Maybe will append not in correct position
    rawData(F("<br/>Not found stored records for "));
    rawData(targetDateAsString);
  }
}

/////////////////////////////////////////////////////////////////////
//                         GENERAL PAGE                           //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendGeneralPage(const String& getParams) {

  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  rawData(F("<fieldset><legend>Clock</legend>"));
  rawData(F("<form action='"));
  rawData(FS(S_url_general));
  rawData(F("' method='post' onSubmit='if (document.getElementById(\"turnToDayModeAt\").value == document.getElementById(\"turnToNightModeAt\").value) { alert(\"Turn times should be different\"); return false;}'>"));
  rawData(F("<table class='grab'>"));
  rawData(F("<tr><td>Day mode at</td><td>"));
  tagInputTime(F("turnToDayModeAt"), 0, upTime);
  rawData(F("</td></tr>"));
  rawData(F("<tr><td>Night mode at</td><td>"));
  tagInputTime(F("turnToNightModeAt"), 0, downTime);
  rawData(F("</td></tr>"));

  rawData(F("<tr><td>Auto adjust time</td><td>"));
  tagInputNumber(F("autoAdjustTimeStampDelta"), 0, -32768, 32767, GB_Controller.getAutoAdjustClockTimeDelta());
  rawData(F(" sec/day"));
  rawData(F("</td></tr>"));

  rawData(F("</table>"));
  rawData(F("<input type='submit' value='Save'>"));
  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  rawData(F("<br/>"));
  if (c_isWifiResponseError)
    return;

  rawData(F("<fieldset><legend>Logger</legend>"));

  rawData(F("<form action='"));
  rawData(FS(S_url_general));
  rawData(F("' method='post' id='resetLogForm' onSubmit='return confirm(\"Delete all stored records?\")'>"));
  rawData(F("<input type='hidden' name='resetStoredLog'/>"));
  rawData(F("</form>"));

  rawData(F("<form action='"));
  rawData(FS(S_url_general));
  rawData(F("' method='post'>"));

  rawData(F("<table class='grab'>"));
  rawData(F("<tr><td colspan='2'>"));
  tagCheckbox(F("isStoreLogRecordsEnabled"), F("Enable logger"), GB_StorageHelper.isStoreLogRecordsEnabled());
  rawData(F("<div class='description'>Stored records ["));
  rawData(GB_StorageHelper.getLogRecordsCount());
  rawData('/');
  rawData(GB_StorageHelper.getLogRecordsCapacity());
  if (GB_StorageHelper.isLogOverflow()) {
    rawData(F(", <span class='red'>overflow</span>"));
  }
  rawData(F("]</div>"));
  rawData(F("</td></tr>"));

  rawData(F("<tr><td>"));
  rawData(F("<input type='submit' value='Save'>"));
  rawData(F("</td><td class='align_right'>"));
  rawData(F("<input form='resetLogForm' type='submit' value='Clear all stored records'/>"));
  rawData(F("</td></tr>"));
  rawData(F("</table>"));

  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  if (GB_StorageHelper.isUseThermometer()) {

    rawData(F("<br/>"));
    if (c_isWifiResponseError)
      return;

    byte normalTemperatueDayMin, normalTemperatueDayMax,
        normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue;
    GB_StorageHelper.getTemperatureParameters(normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue);

    rawData(F("<fieldset><legend>Thermometer</legend>"));
    rawData(F("<form action='"));
    rawData(FS(S_url_general));
    rawData(F("' method='post' onSubmit='if (document.getElementById(\"normalTemperatueDayMin\").value >= document.getElementById(\"normalTemperatueDayMax\").value "));
    rawData(F("|| document.getElementById(\"normalTemperatueNightMin\").value >= document.getElementById(\"normalTemperatueDayMax\").value) { alert(\"Temperature ranges are incorrect\"); return false;}'>"));
    rawData(F("<table>"));

    rawData(F("<tr><td>Normal Day temperature</td><td>"));
    tagInputNumber(F("normalTemperatueDayMin"), 0, 1, 50, normalTemperatueDayMin);
    rawData(F(" .. "));
    tagInputNumber(F("normalTemperatueDayMax"), 0, 1, 50, normalTemperatueDayMax);
    rawData(F("&deg;C</td></tr>"));
    rawData(F("<tr><td>Normal Night temperature</td><td>"));
    tagInputNumber(F("normalTemperatueNightMin"), 0, 1, 50, normalTemperatueNightMin);
    rawData(F(" .. "));
    tagInputNumber(F("normalTemperatueNightMax"), 0, 1, 50, normalTemperatueNightMax);
    rawData(F("&deg;C</td></tr>"));
    rawData(F("<tr><td>Critical temperature</td><td>"));
    tagInputNumber(F("criticalTemperatue"), 0, 1, 50, criticalTemperatue);
    rawData(F("&deg;C</td></tr>"));
    rawData(F("</table>"));
    rawData(F("<input type='submit' value='Save'>"));
    rawData(F("</form>"));
    rawData(F("</fieldset>"));
  }
}

/////////////////////////////////////////////////////////////////////
//                         WATERING PAGE                           //
/////////////////////////////////////////////////////////////////////

byte WebServerClass::getWateringIndexFromUrl(const String& url) {

  if (!StringUtils::flashStringStartsWith(url, FS(S_url_watering))) {
    return 0xFF;
  }

  String urlSuffix = url.substring(StringUtils::flashStringLength(FS(S_url_watering)));
  if (urlSuffix.length() == 0) {
    return 0;
  }

  urlSuffix = urlSuffix.substring(urlSuffix.length() - 1);  // remove left slash

  byte wateringSystemIndex = urlSuffix.toInt();
  wateringSystemIndex--;
  if (wateringSystemIndex < 0 || wateringSystemIndex >= MAX_WATERING_SYSTEMS_COUNT) {
    return 0xFF;
  }
  return wateringSystemIndex;
}

void WebServerClass::sendWateringPage(const String& url, byte wsIndex) {

  String actionURL;
  actionURL += StringUtils::flashStringLoad(FS(S_url_watering));
  if (wsIndex > 0) {
    actionURL += '/';
    actionURL += (wsIndex + 1);
  }

  BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

  // run Dry watering form
  rawData(F("<form action='"));
  rawData(actionURL);
  rawData(F("' method='post' id='runDryWateringNowForm' onSubmit='return confirm(\"Start manually Dry watering during "));
  rawData(wsp.dryWateringDuration);
  rawData(F(" sec ?\")'>"));
  rawData(F("<input type='hidden' name='runDryWateringNow'>"));
  rawData(F("</form>"));

  // clearLastWateringTimeForm
  rawData(F("<form action='"));
  rawData(actionURL);
  rawData(F("' method='post' id='clearLastWateringTimeForm' onSubmit='return confirm(\"Clear last watering time?\")'>"));
  rawData(F("<input type='hidden' name='clearLastWateringTime'>"));
  rawData(F("</form>"));

#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME
  if (g_useSerialMonitor) {
    Serial.println(); // We cut log stream to show wet status in new line
  }
#endif
  GB_Watering.updateWetSatus();
  byte currentValue = GB_Watering.getCurrentWetSensorValue(wsIndex);
  WateringEvent* currentStatus = GB_Watering.getCurrentWetSensorStatus(wsIndex);

  // General form
  if (c_isWifiResponseError)
    return;

  rawData(F("<fieldset><legend>General</legend>"));
  rawData(F("<form action='"));
  rawData(actionURL);
  rawData(F("' method='post'>"));

  tagCheckbox(F("isWetSensorConnected"), F("Wet sensor connected"), wsp.boolPreferencies.isWetSensorConnected);

  rawData(F("<div class='description'>Current value [<b>"));
  if (GB_Watering.isWetSensorValueReserved(currentValue)) {
    rawData(F("N/A"));
  }
  else {
    rawData(currentValue);
  }
  rawData(F("</b>], state [<b>"));
  rawData(currentStatus->shortDescription);
  rawData(F("</b>]</div>"));

  tagCheckbox(F("isWaterPumpConnected"), F("Watering Pump connected"), wsp.boolPreferencies.isWaterPumpConnected);
  rawData(F("<div class='description'>Last watering&emsp;"));
  rawData(GB_Watering.getLastWateringTimeStampByIndex(wsIndex));
  rawData(F("<br/>Current time &nbsp;&emsp;"));
  rawData(now());
  rawData(F("<br/>"));
  rawData(F("Next watering&emsp;"));
  rawData(GB_Watering.getNextWateringTimeStampByIndex(wsIndex));
  rawData(F("</div>"));

  rawData(F("<table><tr><td>"));
  rawData(F("Daily watering at"));
  rawData(F("</td><td>"));
  tagInputTime(F("startWateringAt"), 0, wsp.startWateringAt);
  rawData(F("</td></tr></table>"));

  tagCheckbox(F("useWetSensorForWatering"), F("Use Wet sensor to detect State"), wsp.boolPreferencies.useWetSensorForWatering);
  rawData(F("<div class='description'>If Wet sensor [Not connected], [In Air], [Disabled] or [Unstable], then [Dry watering] duration will be used</div>"));

  tagCheckbox(F("skipNextWatering"), F("Skip next scheduled watering"), wsp.boolPreferencies.skipNextWatering);
  rawData(F("<div class='description'>Useful after manual watering</div>"));

  rawData(F("<table class='grab'>"));
  rawData(F("<tr><td>"));
  rawData(F("<input type='submit' value='Save'></td><td class='align_right'>"));
  rawData(F("<input type='submit' form='clearLastWateringTimeForm' value='Clear last watering'>"));
  rawData(F("<br><input type='submit' form='runDryWateringNowForm' value='Start watering now'>"));
  rawData(F("</td></tr>"));
  rawData(F("</table>"));

  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  rawData(F("<br/>"));

  // Wet sensor
  if (c_isWifiResponseError)
    return;

  rawData(F("<fieldset><legend>Wet sensor</legend>"));
  rawData(F("<form action='"));
  rawData(actionURL);
  rawData(F("' method='post'>"));

  rawData(F("<table class='grab'>"));

  rawData(F("<tr><th>State</th><th>Value range</th></tr>"));

  rawData(F("<tr><td>In Air</td><td>"));
  tagInputNumber(F("inAirValue"), 0, 1, 255, wsp.inAirValue);
  rawData(F(" .. [255]</td></tr>"));

  rawData(F("<tr><td>Very Dry</td><td>"));
  tagInputNumber(F("veryDryValue"), 0, 1, 255, wsp.veryDryValue);
  rawData(F(" .. [In Air]</td></tr>"));

  rawData(F("<tr><td>Dry</td><td>"));
  tagInputNumber(F("dryValue"), 0, 1, 255, wsp.dryValue);
  rawData(F(" .. [Very Dry]</td></tr>"));

  rawData(F("<tr><td>Normal</td><td>"));
  tagInputNumber(F("normalValue"), 0, 1, 255, wsp.normalValue);
  rawData(F(" .. [Dry]</td></tr>"));

  rawData(F("<tr><td>Wet</td><td>"));
  tagInputNumber(F("wetValue"), 0, 1, 255, wsp.wetValue);
  rawData(F(" .. [Normal]</td></tr>"));

  rawData(F("<tr><td>Very Wet</td><td>"));
  tagInputNumber(F("veryWetValue"), 0, 1, 255, wsp.veryWetValue);
  rawData(F(" .. [Wet]</td></tr>"));

  rawData(F("<tr><td>Short circit</td><td>[0] .. [Very Wet]</td></tr>"));

  rawData(F("</table>"));
  rawData(F("<input type='submit' value='Save'>"));
  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  rawData(F("<br/>"));

  //Watering pump
  if (c_isWifiResponseError)
    return;

  rawData(F("<fieldset><legend>Watering pump</legend>"));

  rawData(F("<form action='"));
  rawData(actionURL);
  rawData(F("' method='post'>"));

  rawData(F("<table class='grab'>"));

  rawData(F("<tr><th>State</th><th>Duration</th></tr>"));

  rawData(F("<tr><td>Dry watering</td><td>"));
  tagInputNumber(F("dryWateringDuration"), 0, 1, 255, wsp.dryWateringDuration);
  rawData(F(" sec</td></tr>"));

  rawData(F("<tr><td>Very Dry watering</td><td>"));
  tagInputNumber(F("veryDryWateringDuration"), 0, 1, 255, wsp.veryDryWateringDuration);
  rawData(F(" sec</td></tr>"));

  rawData(F("</table>"));
  rawData(F("<input type='submit' value='Save'>"));
  rawData(F("</form>"));

  rawData(F("</fieldset>"));

}

/////////////////////////////////////////////////////////////////////
//                        HARDWARE PAGES                           //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendHardwarePage(const String& getParams) {

  rawData(F("<fieldset><legend>General</legend>"));

  rawData(F("<form action='"));
  rawData(FS(S_url_hardware));
  rawData(F("' method='post'>"));

  tagCheckbox(F("useLight"), F("Use Light"), GB_Controller.isUseLight());
  rawData(F("<div class='description'> </div>"));

  tagCheckbox(F("useFan"), F("Use Fan"), GB_Controller.isUseFan());
  rawData(F("<div class='description'> </div>"));

  tagCheckbox(F("useRTC"), F("Use Real-time clock DS1307"), GB_Controller.isUseRTC());
  rawData(F("<div class='description'>Current state [<b>"));
  spanTag_RedIfTrue(
      GB_Controller.isRTCPresent() ? F("Connected") : F("Not connected"), GB_Controller.isUseRTC() && !GB_Controller.isRTCPresent());
  rawData(F("</b>]</div>"));

  tagCheckbox(F("isEEPROM_AT24C32_Connected"), F("Use external AT24C32 EEPROM"), GB_StorageHelper.isUseExternal_EEPROM_AT24C32());
  rawData(F("<div class='description'>Current state [<b>"));
  spanTag_RedIfTrue(
      EEPROM_AT24C32.isPresent() ? F("Connected") : F("Not connected"), GB_StorageHelper.isUseExternal_EEPROM_AT24C32() && !EEPROM_AT24C32.isPresent());
  rawData(F("</b>]<br/>On change stored records maybe will lost</div>"));

  tagCheckbox(F("useThermometer"), F("Use Thermometer DS18B20, DS18S20 or DS1822"), GB_StorageHelper.isUseThermometer());
  rawData(F("<div class='description'>Current state [<b>"));
  spanTag_RedIfTrue(
      GB_Thermometer.isPresent() ? F("Connected") : F("Not connected"), GB_StorageHelper.isUseThermometer() && !GB_Thermometer.isPresent());
  rawData(F("</b>]</div>"));

  rawData(F("<input type='submit' value='Save'>"));

  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  rawData(F("<br/>"));
  if (c_isWifiResponseError)
    return;

  boolean isWifiStationMode = GB_StorageHelper.isWifiStationMode();
  rawData(F("<fieldset><legend>Wi-Fi</legend>"));
  rawData(F("<form action='"));
  rawData(FS(S_url_hardware));
  rawData(F("' method='post'>"));
  tagRadioButton(F("isWifiStationMode"), F("Create new Network"), F("0"), !isWifiStationMode);
  rawData(F("<div class='description'>Hidden, security WPA2, ip 192.168.0.1</div>"));
  tagRadioButton(F("isWifiStationMode"), F("Join existed Network"), F("1"), isWifiStationMode);
  rawData(F("<table>"));
  rawData(F("<tr><td><label for='wifiSSID'>SSID</label></td><td><input type='text' name='wifiSSID' required value='"));
  rawData(GB_StorageHelper.getWifiSSID());
  rawData(F("' maxlength='"));
  rawData(WIFI_SSID_LENGTH);
  rawData(F("'/></td></tr>"));
  rawData(F("<tr><td><label for='wifiPass'>Password</label></td><td><input type='password' name='wifiPass' value='"));
  rawData(GB_StorageHelper.getWifiPass());
  rawData(F("' maxlength='"));
  rawData(WIFI_PASS_LENGTH);
  rawData(F("'/></td></tr>"));
  rawData(F("</table>"));
  rawData(F("<input type='submit' value='Save'> <small>and reboot Wi-Fi manually</small>"));
  rawData(F("</form>"));
  rawData(F("</fieldset>"));

}

void WebServerClass::sendOtherPage(const String& getParams) {
  //rawData(F("<fieldset><legend>Other</legend>"));  
  rawData(F("<table style='vertical-align:top; border-spacing:0px;'>"));

  rawData(F("<tr><td colspan ='2'>"));
  rawData(F("<a href='"));
  rawData(FS(S_url_dump_internal));
  rawData(F("'>View internal Arduino dump</a>"));
  rawData(F("</td></tr>"));
  if (EEPROM_AT24C32.isPresent()) {
    rawData(F("<tr><td colspan ='2'>"));
    rawData(F("<a href='"));
    rawData(FS(S_url_dump_AT24C32));
    rawData(F("'>View external AT24C32 dump</a>"));
    rawData(F("</td></tr>"));
  }

  rawData(F("<tr><td colspan ='2'><br/></td></tr>"));

  rawData(F("<tr><td>"));
  rawData(F("<form action='"));
  rawData(FS(S_url_root));
  rawData(F("' method='post' onSubmit='return confirm(\"Reboot Growbox?\")'>"));
  rawData(F("<input type='hidden' name='rebootController'/><input type='submit' value='Reboot Growbox'>"));
  rawData(F("</form>"));
  rawData(F("</td><td>"));
  rawData(F(" <small>and update page manually</small>"));
  rawData(F("</td></tr>"));

  rawData(F("<tr><td>"));
  rawData(F("<form action='"));
  rawData(FS(S_url_root));
  rawData(F("' method='post' onSubmit='return confirm(\"Reset Firmware?\")'>"));
  rawData(F("<input type='hidden' name='resetFirmware'/><input type='submit' value='Reset Firmware'>"));
  rawData(F("</form>"));
  rawData(F("</td><td>"));
  rawData(F("<small>and update page manually. Default Wi-Fi SSID and pass will be used  </small>"));
  rawData(F("</td></tr>"));

  rawData(F("</table>"));
  //rawData(F("</fieldset>"));
}

void WebServerClass::sendStorageDumpPage(const String& getParams, boolean isInternal) {

  String paramValue;
  byte rangeStart = 0x0; //0x[0]00
  byte rangeEnd = 0x0;   //0x[0]FF
  byte temp;
  if (searchHttpParamByName(getParams, F("rangeStart"), paramValue)) {
    if (paramValue.length() == 1) {
      temp = StringUtils::hexCharToByte(paramValue[0]);
      if (temp != 0xFF) {
        rangeStart = temp;
      }
    }
  }
  if (searchHttpParamByName(getParams, F("rangeEnd"), paramValue)) {
    if (paramValue.length() == 1) {
      temp = StringUtils::hexCharToByte(paramValue[0]);
      if (temp != 0xFF) {
        rangeEnd = temp;
      }
    }
  }
  if (rangeStart > rangeEnd) {
    rangeEnd = rangeStart;
  }

  //  if(isInternal){
  //    rawData(F("Internal Arduino dump"));
  //  } else {
  //    rawData(F("External AT24C32 dump"));
  //  }
  //rawData(F("<br/>"));

  rawData(F("<form action='"));
  if (isInternal) {
    rawData(FS(S_url_dump_internal));
  }
  else {
    rawData(FS(S_url_dump_AT24C32));
  }
  rawData(F("' method='get' onSubmit='if (document.getElementById(\"rangeStartCombobox\").value > document.getElementById(\"rangeEndCombobox\").value){ alert(\"Address range is incorrect\"); return false;}'>"));
  rawData(F("Address from "));
  rawData(F("<select id='rangeStartCombobox' name='rangeStart'>"));

  String stringValue;
  for (byte counter = 0; counter < 0x10; counter++) {
    stringValue = StringUtils::byteToHexString(counter, true);
    stringValue += '0';
    stringValue += '0';
    tagOption(String(counter, HEX), stringValue, rangeStart == counter);
  }
  rawData(F("</select>"));
  rawData(F(" to "));
  rawData(F("<select id='rangeEndCombobox' name='rangeEnd'>"));

  for (byte counter = 0; counter < 0x10; counter++) {
    stringValue = StringUtils::byteToHexString(counter, true);
    stringValue += 'F';
    stringValue += 'F';
    tagOption(String(counter, HEX), stringValue, rangeEnd == counter);
  }
  rawData(F("</select>"));
  rawData(F("<input type='submit' value='Show'/>"));
  rawData(F("</form>"));

  if (!isInternal && !EEPROM_AT24C32.isPresent()) {
    rawData(F("<br/>External AT24C32 EEPROM not connected"));
    return;
  }

  rawData(F("<table class='grab align_center'><tr><th/>"));
  for (word i = 0; i < 0x10; i++) {
    rawData(F("<th>"));
    String colName(i, HEX);
    colName.toUpperCase();
    rawData(colName);
    rawData(F("</th>"));
  }
  rawData(F("</tr>"));

  word realRangeStart = ((word)(rangeStart) << 8);
  word realRangeEnd = ((word)(rangeEnd) << 8) + 0xFF;
  byte value;
  for (word i = realRangeStart;
      i <= realRangeEnd/*EEPROM_AT24C32.getCapacity()*/; i++) {

    if (c_isWifiResponseError)
      return;

    if (isInternal) {
      value = EEPROM.read(i);
    }
    else {
      value = EEPROM_AT24C32.read(i);
    }

    if (i % 16 == 0) {
      if (i > 0) {
        rawData(F("</tr>"));
      }
      rawData(F("<tr><td><b>"));
      rawData(StringUtils::byteToHexString(i / 16));
      rawData(F("</b></td>"));
    }
    rawData(F("<td>"));
    rawData(StringUtils::byteToHexString(value));
    rawData(F("</td>"));

  }
  rawData(F("</tr></table>"));
}

/////////////////////////////////////////////////////////////////////
//                          POST HANDLING                          //
/////////////////////////////////////////////////////////////////////

String WebServerClass::applyPostParams(const String& url, const String& postParams) {
  //String queryStr;
  word index = 0;
  String name, value;
  while (getHttpParamByIndex(postParams, index, name, value)) {
    //if (queryStr.length() > 0){
    //  queryStr += '&';
    //}
    //queryStr += name;
    //queryStr += '=';   
    //queryStr += applyPostParam(name, value);

    boolean result = applyPostParam(url, name, value);

    if (g_useSerialMonitor) {
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

boolean WebServerClass::applyPostParam(const String& url, const String& name, const String& value) {

  if (StringUtils::flashStringEquals(name, F("isWifiStationMode"))) {
    if (value.length() != 1) {
      return false;
    }
    GB_StorageHelper.setWifiStationMode(value[0] == '1');

  }
  else if (StringUtils::flashStringEquals(name, F("wifiSSID"))) {
    GB_StorageHelper.setWifiSSID(value);

  }
  else if (StringUtils::flashStringEquals(name, F("wifiPass"))) {
    GB_StorageHelper.setWifiPass(value);

  }
  else if (StringUtils::flashStringEquals(name, F("resetStoredLog"))) {
    GB_StorageHelper.resetStoredLog();
  }
  else if (StringUtils::flashStringEquals(name, F("isStoreLogRecordsEnabled"))) {
    if (value.length() != 1) {
      return false;
    }
    GB_StorageHelper.setStoreLogRecordsEnabled(value[0] == '1');
  }
  else if (StringUtils::flashStringEquals(name, F("rebootController"))) {
    GB_Controller.rebootController();
  }
  else if (StringUtils::flashStringEquals(name, F("resetFirmware"))) {
    GB_StorageHelper.resetFirmware();
    GB_Controller.rebootController();
  }

  else if (StringUtils::flashStringEquals(name, F("turnToDayModeAt")) || StringUtils::flashStringEquals(name, F("turnToNightModeAt"))) {
    word timeValue = getTimeFromInput(value);
    if (timeValue == 0xFFFF) {
      return false;
    }

    if (StringUtils::flashStringEquals(name, F("turnToDayModeAt"))) {
      GB_StorageHelper.setTurnToDayModeTime(timeValue / 60, timeValue % 60);
    }
    else if (StringUtils::flashStringEquals(name, F("turnToNightModeAt"))) {
      GB_StorageHelper.setTurnToNightModeTime(timeValue / 60, timeValue % 60);
    }
    c_isWifiForceUpdateGrowboxState = true;
  }
  else if (StringUtils::flashStringEquals(name, F("normalTemperatueDayMin")) || StringUtils::flashStringEquals(name, F("normalTemperatueDayMax")) || StringUtils::flashStringEquals(name, F("normalTemperatueNightMin")) || StringUtils::flashStringEquals(name, F("normalTemperatueNightMax")) || StringUtils::flashStringEquals(name, F("criticalTemperatue"))) {
    byte temp = value.toInt();
    if (temp == 0) {
      return false;
    }

    if (StringUtils::flashStringEquals(name, F("normalTemperatueDayMin"))) {
      GB_StorageHelper.setNormalTemperatueDayMin(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("normalTemperatueDayMax"))) {
      GB_StorageHelper.setNormalTemperatueDayMax(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("normalTemperatueNightMin"))) {
      GB_StorageHelper.setNormalTemperatueNightMin(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("normalTemperatueNightMax"))) {
      GB_StorageHelper.setNormalTemperatueNightMax(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("criticalTemperatue"))) {
      GB_StorageHelper.setCriticalTemperatue(temp);
    }

    c_isWifiForceUpdateGrowboxState = true;
  }

  else if (StringUtils::flashStringEquals(name, F("isWetSensorConnected")) || StringUtils::flashStringEquals(name, F("isWaterPumpConnected")) || StringUtils::flashStringEquals(name, F("useWetSensorForWatering")) || StringUtils::flashStringEquals(name, F("skipNextWatering"))) {

    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF) {
      return false;
    }
    if (value.length() != 1) {
      return false;
    }
    boolean boolValue = (value[0] == '1');

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (StringUtils::flashStringEquals(name, F("isWetSensorConnected"))) {
      wsp.boolPreferencies.isWetSensorConnected = boolValue;
    }
    else if (StringUtils::flashStringEquals(name, F("isWaterPumpConnected"))) {
      wsp.boolPreferencies.isWaterPumpConnected = boolValue;
      wsp.lastWateringTimeStamp = 0;
    }
    else if (StringUtils::flashStringEquals(name, F("useWetSensorForWatering"))) {
      wsp.boolPreferencies.useWetSensorForWatering = boolValue;
    }
    else if (StringUtils::flashStringEquals(name, F("skipNextWatering"))) {
      wsp.boolPreferencies.skipNextWatering = boolValue;
    }
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);

    if (StringUtils::flashStringEquals(name, F("isWaterPumpConnected"))) {
      GB_Watering.updateWateringSchedule();
    }
  }
  else if (StringUtils::flashStringEquals(name, F("inAirValue")) || StringUtils::flashStringEquals(name, F("veryDryValue")) || StringUtils::flashStringEquals(name, F("dryValue")) || StringUtils::flashStringEquals(name, F("normalValue")) || StringUtils::flashStringEquals(name, F("wetValue")) || StringUtils::flashStringEquals(name, F("veryWetValue"))) {

    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF) {
      return false;
    }
    byte intValue = value.toInt();
    if (intValue == 0) {
      return false;
    }

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (StringUtils::flashStringEquals(name, F("inAirValue"))) {
      wsp.inAirValue = intValue;
    }
    else if (StringUtils::flashStringEquals(name, F("veryDryValue"))) {
      wsp.veryDryValue = intValue;
    }
    else if (StringUtils::flashStringEquals(name, F("dryValue"))) {
      wsp.dryValue = intValue;
    }
    else if (StringUtils::flashStringEquals(name, F("normalValue"))) {
      wsp.normalValue = intValue;
    }
    else if (StringUtils::flashStringEquals(name, F("wetValue"))) {
      wsp.wetValue = intValue;
    }
    else if (StringUtils::flashStringEquals(name, F("veryWetValue"))) {
      wsp.veryWetValue = intValue;
    }
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);
  }
  else if (StringUtils::flashStringEquals(name, F("dryWateringDuration")) || StringUtils::flashStringEquals(name, F("veryDryWateringDuration"))) {

    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF) {
      return false;
    }
    byte intValue = value.toInt();
    if (intValue == 0) {
      return false;
    }

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (StringUtils::flashStringEquals(name, F("dryWateringDuration"))) {
      wsp.dryWateringDuration = intValue;
    }
    else if (StringUtils::flashStringEquals(name, F("veryDryWateringDuration"))) {
      wsp.veryDryWateringDuration = intValue;
    }
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);
  }
  else if (StringUtils::flashStringEquals(name, F("startWateringAt"))) {
    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF) {
      return false;
    }
    word timeValue = getTimeFromInput(value);
    if (timeValue == 0xFFFF) {
      return false;
    }
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    wsp.startWateringAt = timeValue;
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);

    GB_Watering.updateWateringSchedule();
  }
  else if (StringUtils::flashStringEquals(name, F("runDryWateringNow"))) {
    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF) {
      return false;
    }
    GB_Watering.turnOnWaterPumpManual(wsIndex);
  }
  else if (StringUtils::flashStringEquals(name, F("clearLastWateringTime"))) {
    byte wsIndex = getWateringIndexFromUrl(url);
    if (wsIndex == 0xFF) {
      return false;
    }
    GB_Watering.turnOnWaterPumpManual(wsIndex);

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    wsp.lastWateringTimeStamp = 0;
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);

    GB_Watering.updateWateringSchedule();
  }
  else if (StringUtils::flashStringEquals(name, F("setClockTime"))) {
    time_t newTimeStamp = strtoul(value.c_str(), NULL, 0);
    if (newTimeStamp == 0) {
      return false;
    }
    GB_Controller.setClockTime(newTimeStamp + UPDATE_WEB_SERVER_AVERAGE_PAGE_LOAD_DELAY);

    c_isWifiForceUpdateGrowboxState = true; // Switch to Day/Night mode
  }
  else if (StringUtils::flashStringEquals(name, F("useRTC"))) {
    if (value.length() != 1) {
      return false;
    }
    boolean boolValue = (value[0] == '1');
    GB_Controller.setUseRTC(boolValue);

    c_isWifiForceUpdateGrowboxState = true; // Switch to Day/Night mode
  }
  else if (StringUtils::flashStringEquals(name, F("autoAdjustTimeStampDelta"))) {
    if (value.length() == 0) {
      return false;
    }
    boolean isZero = false;
    if (value.length() == 1) {
      if (value[0] == '0') {
        isZero = true;
      }
    }
    int16_t intValue = value.toInt();
    if (intValue == 0 && !isZero) {
      return false;
    }
    GB_Controller.setAutoAdjustClockTimeDelta(intValue);
  }
  else if (StringUtils::flashStringEquals(name, F("isEEPROM_AT24C32_Connected"))) {
    if (value.length() != 1) {
      return false;
    }
    boolean boolValue = (value[0] == '1');
    GB_StorageHelper.setUseExternal_EEPROM_AT24C32(boolValue);
  }
  else if (StringUtils::flashStringEquals(name, F("useThermometer"))) {
    if (value.length() != 1) {
      return false;
    }
    boolean boolValue = (value[0] == '1');
    GB_StorageHelper.setUseThermometer(boolValue);

    c_isWifiForceUpdateGrowboxState = true;
  }
  else if (StringUtils::flashStringEquals(name, F("useLight"))) {
    if (value.length() != 1) {
      return false;
    }
    boolean boolValue = (value[0] == '1');
    GB_Controller.setUseLight(boolValue);

    c_isWifiForceUpdateGrowboxState = true;
  }
  else if (StringUtils::flashStringEquals(name, F("useFan"))) {
    if (value.length() != 1) {
      return false;
    }
    boolean boolValue = (value[0] == '1');
    GB_Controller.setUseFan(boolValue);

    c_isWifiForceUpdateGrowboxState = true;
  }
  else {
    return false;
  }

  return true;
}

WebServerClass GB_WebServer;

