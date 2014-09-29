#include "WebServer.h"

#include <MemoryFree.h>
#include "Controller.h"
#include "StorageHelper.h" 
#include "Thermometer.h" 
#include "Watering.h"
#include "RAK410_XBeeWifi.h" 
#include "EEPROM_AT24C32.h" 

/////////////////////////////////////////////////////////////////////
//                        COMMON FOR ALL PAGES                     //
/////////////////////////////////////////////////////////////////////

void WebServerClass::httpProcessGet(const String& url, const String& getParams) {

  byte wsIndex = getWateringIndexFromUrl(url); // FF ig not watering system

  boolean isStatusPage = StringUtils::flashStringEquals(url, FS(S_URL_STATUS));
  boolean isLogPage = StringUtils::flashStringEquals(url, FS(S_URL_DAILY_LOG));

  boolean isGeneralOptionsPage = StringUtils::flashStringEquals(url, FS(S_URL_GENERAL_OPTIONS));
  boolean isGeneralOptionsSummaryPage = StringUtils::flashStringEquals(url, FS(S_URL_GENERAL_OPTIONS_SUMMARY));
  boolean isWateringPage = (wsIndex != 0xFF);
  boolean isHardwarePage = StringUtils::flashStringEquals(url, FS(S_URL_HARDWARE));
  boolean isOtherPage = StringUtils::flashStringEquals(url, FS(S_URL_OTHER_PAGE));
  boolean isDumpInternal = StringUtils::flashStringEquals(url, FS(S_URL_DUMP_INTERNAL));
  boolean isDumpAT24C32 = StringUtils::flashStringEquals(url, FS(S_URL_DUMP_AT24C32));
  boolean isPinMapPage = StringUtils::flashStringEquals(url, FS(S_URL_PINMAP));

  boolean isConfigurationPage = (isGeneralOptionsPage || isGeneralOptionsSummaryPage || isWateringPage || isHardwarePage || isOtherPage || isDumpInternal || isDumpAT24C32 || isPinMapPage);

  boolean isValidPage = (isStatusPage || isLogPage || isConfigurationPage);
  if (!isValidPage) {
    httpNotFound();
    return;
  }

  httpPageHeader();

  if (isStatusPage || isWateringPage) {
#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME
    if (g_useSerialMonitor) {
      Serial.println(); // We cut log stream to show wet status in new line
    }
#endif
    GB_Watering.preUpdateWetSatus();
  }

  rawData(F("<!DOCTYPE html>")); // HTML 5
  rawData(F("<html><head>"));{
    rawData(F("<title>Growbox</title>"));
    rawData(F("<meta name='viewport' content='width=device-width, initial-scale=1'/>"));
    rawData(F("<style type='text/css'>"));{
      rawData(F("body{font-family:Arial;max-width:600px; }")); // 350px
      rawData(F("form{margin: 0px;}"));
      rawData(F("dt{font-weight:bold; margin-top:5px;}"));

      rawData(F(".red {color: red;}"));
      rawData(F(".grab {border-spacing:5px;width:100%; }"));
      rawData(F(".align_left{float:left;text-align:left;}"));
      rawData(F(".align_center{float:center;text-align:center;}"));
      rawData(F(".align_right{float:right;text-align:right;}"));
      rawData(F(".description{font-size:small;margin-left:20px;margin-bottom:5px;}"));
    }
    rawData(F("</style>"));
  }

  rawData(F("</head>"));

  rawData(F("<body>"));
  rawData(F("<h1>Growbox</h1>"));
  if (isCriticalErrorOnStatusPage()){
    rawData(F("<hr/>"));
    spanTag_RedIfTrue(F("Critical system error"), true);
    if (!isStatusPage) {
      spanTag_RedIfTrue(F(". Check Status page"), true);
    }
  }
  rawData(F("<form>"));   // HTML Validator warning
  tagButton(FS(S_URL_STATUS), F("Status"), isStatusPage);
  tagButton(FS(S_URL_DAILY_LOG), F("Daily log"), isLogPage);
  rawData(F("<select id='configPageSelect' onchange='g_onChangeConfigPageSelect();' "));
  if (isConfigurationPage) {
    rawData(F(" style='text-decoration: underline;'"));
  }
  rawData('>');
  tagOption(F(""), F("Select options page"), !isConfigurationPage, true);
  tagOption(FS(S_URL_GENERAL_OPTIONS), F("General options"), isGeneralOptionsPage);
  if (isGeneralOptionsSummaryPage){
    tagOption(FS(S_URL_GENERAL_OPTIONS_SUMMARY), F("General options: Summary"), isGeneralOptionsSummaryPage);
  }
  for (byte i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++) {
    String url = StringUtils::flashStringLoad(FS(S_URL_WATERING));
    if (i > 0) {
      url += '/';
      url += (i + 1);
    }
    String description = StringUtils::flashStringLoad(F("Watering system #"));
    description += (i + 1);
    tagOption(url, description, (wsIndex == i));
  }

  tagOption(FS(S_URL_HARDWARE), F("Hardware"), isHardwarePage);
  tagOption(FS(S_URL_OTHER_PAGE), F("Other"), isOtherPage);
  if (isDumpInternal || isDumpAT24C32) {
    tagOption(FS(S_URL_DUMP_INTERNAL), F("Other: Arduino dump"), isDumpInternal);
    if (EEPROM_AT24C32.isPresent()) {
      tagOption(FS(S_URL_DUMP_AT24C32), F("Other: AT24C32 dump"), isDumpAT24C32);
    }
  }
  else if (isPinMapPage) {
    tagOption(FS(S_URL_PINMAP), F("Other: Pin map"), isPinMapPage);
  }
  rawData(F("</select>"));
  rawData(F("</form>"));

  rawData(F("<script type='text/javascript'>"));
  rawData(F("var g_configPageSelect = document.getElementById('configPageSelect');"));
  rawData(F("var g_configPageSelectedDefault = g_configPageSelect.value;"));
  rawData(F("function g_onChangeConfigPageSelect(event) {"));{
    rawData(F("var newValue = g_configPageSelect.value;"));
    rawData(F("g_configPageSelect.value = g_configPageSelectedDefault;"));
    rawData(F("location = newValue;"));
  }
  rawData(F("}"));
  rawData(F("</script>"));

  rawData(F("<hr/>"));

  if (isStatusPage) {
    sendStatusPage();
  }
  else if (isLogPage) {
    sendLogPage(getParams);
  }
  else if (isGeneralOptionsPage) {
    sendGeneralOptionsPage(getParams);
  }
  else if (isGeneralOptionsSummaryPage) {
    sendGeneralOptionsSummaryPage();
  }
  else if (isWateringPage) {
    sendWateringOptionsPage(url, wsIndex);
  }
  else if (isHardwarePage) {
    sendHardwareOptionsPage(getParams);
  }
  else if (isOtherPage) {
    sendOtherOptionsPage(getParams);
  }
  else if (isDumpInternal || isDumpAT24C32) {
    sendStorageDumpPage(getParams, isDumpInternal);
  }
  else if (isPinMapPage) {
    sendPinMapPage();
  }

  rawData(F("</body></html>"));

  httpPageComplete();

}

/////////////////////////////////////////////////////////////////////
//                          STATUS PAGE                            //
/////////////////////////////////////////////////////////////////////

boolean WebServerClass::isCriticalErrorOnStatusPage(){
  return (
      (GB_Controller.isBreezeFatalError()) ||
      (GB_Controller.isUseRTC() && (!GB_Controller.isRTCPresent() || GB_Controller.isClockNeedsSync() || GB_Controller.isClockNotSet())) ||
      (GB_StorageHelper.isUseExternal_EEPROM_AT24C32() && !EEPROM_AT24C32.isPresent()) ||
      (GB_Thermometer.isUseThermometer() && !GB_Thermometer.isPresent())
  );
}

void WebServerClass::sendStatusPage() {

  rawData(F("<dl>"));

  rawData(F("<dt>General</dt>"));

  if (GB_Controller.isBreezeFatalError()) {
    rawData(F("<dd>"));
    spanTag_RedIfTrue(F("No breeze"), true);
    rawData(F("</dd>"));
  }

  boolean isDayInGrowbox = GB_Controller.isDayInGrowbox();

  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);
  word dayPeriod = getWordTimePeriodinDay(upTime, downTime); // Can be 24:00
  word nightPeriod = 24 * 60 - dayPeriod; // Can be 00:00
  rawData(F("<dd>Mode: "));
  rawData(isDayInGrowbox ? F("day") : F("night"));
  rawData(F(", "));
  if (isDayInGrowbox) {
    rawData(F("<b>"));
  }
  rawData(StringUtils::wordTimeToString(dayPeriod));
  if (isDayInGrowbox) {
    rawData(F("</b>"));
  }
  rawData(F("/"));
  if (!isDayInGrowbox) {
    rawData(F("<b>"));
  }
  rawData(StringUtils::wordTimeToString(nightPeriod));
  if (!isDayInGrowbox) {
    rawData(F("</b>"));
  }
  rawData(F("</dd>"));

  if (GB_Controller.isUseLight()) {
    rawData(F("<dd>Light: "));
    rawData(GB_Controller.isLightTurnedOn() ? F("on") : F("off"));
    rawData(F("</dd>"));
  }

  if (GB_Controller.isUseFan()) {
    rawData(F("<dd>Fan: "));
    printFanSpeed(GB_Controller.getFanSpeedValue());
    rawData(F("</dd>"));
  }

  if (GB_Controller.isUseHeater()) {
    rawData(F("<dd>Heater: "));
    rawData(GB_Controller.isHeaterTurnedOn() ? F("on") : F("off"));
    rawData(F("</dd>"));
  }

  if (GB_Controller.isUseRTC()) {
    if (!GB_Controller.isRTCPresent()) {
      rawData(F("<dd>Real-time clock: "));
      spanTag_RedIfTrue(F("not connected"), true);
      rawData(F("</dd>"));
    }
    else if (GB_Controller.isClockNeedsSync() || GB_Controller.isClockNotSet()) {
      rawData(F("<dd>Clock: "));
      if (GB_Controller.isClockNotSet()) {
        spanTag_RedIfTrue(F("not set"), true);
      }
      if (GB_Controller.isClockNeedsSync()) {
        spanTag_RedIfTrue(F("needs sync"), true);
      }
      rawData(F("</dd>"));
    }
  }

  rawData(F("<dd>Startup: "));
  rawData(GB_StorageHelper.getStartupTimeStamp(), false, true);
  rawData(F("</dd>"));

  rawData(F("<dd>Date &amp; time: "));
  rawData(F("<span id='growboxTimeStampId'>Loading...</span>"));
  rawData(F("<br/><small><span id='diffTimeStampId'>Wait just a little</span></small>"));
  rawData(F("<form action='"));
  rawData(FS(S_URL_STATUS));
  rawData(F("' method='post' onSubmit='return g_checkBeforeSetClockTime()'>"));
  rawData(F("<input type='hidden' name='setClockTime' id='setClockTimeInput'>"));
  rawData(F("<input type='submit' value='Sync'>"));
  rawData(F("</form>"));
  rawData(F("</dd>"));

  if (c_isWifiResponseError) {
    return;
  }

  if (GB_Thermometer.isUseThermometer()) {
    float lastTemperature, statisticsTemperature;
    int statisticsCount;
    GB_Thermometer.getStatistics(lastTemperature, statisticsTemperature, statisticsCount);

    rawData(F("<dt>Thermometer</dt>"));
    if (!GB_Thermometer.isPresent()) {
      spanTag_RedIfTrue(F("<dd>Not connected</dd>"), true);
    }
    rawData(F("<dd>Last check: "));
    printTemperatue(lastTemperature);
    rawData(F("</dd>"));
    rawData(F("<dd>Current: "));
    printTemperatue(GB_Thermometer.getHardwareTemperature());
    rawData(F("</dd>"));
    rawData(F("<dd>Forecast: "));
    printTemperatue(statisticsTemperature);
    rawData(F(" ("));
    rawData(statisticsCount);
    rawData(F(" measurement"));
    if (statisticsCount > 1) {
      rawData('s');
    }
    rawData(F(")</dd>"));
  }
  if (c_isWifiResponseError) {
    return;
  }

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
      rawData(F("<dd>Wet: "));
      WateringEvent* currentStatus = GB_Watering.getCurrentWetSensorStatus(wsIndex);
      spanTag_RedIfTrue(currentStatus->shortDescription, currentStatus != &WATERING_EVENT_WET_SENSOR_NORMAL);

      byte value = GB_Watering.getCurrentWetSensorValue(wsIndex);
      if (!GB_Watering.isWetSensorValueReserved(value)) {
        rawData(F(", value "));
        rawData(value);
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

  rawData(F("<dt>Logger</dt>"));
  if (GB_StorageHelper.isUseExternal_EEPROM_AT24C32() && !EEPROM_AT24C32.isPresent()) {
    rawData(F("<dd>External AT24C32 EEPROM: "));
    spanTag_RedIfTrue(F("not connected"), true);
    rawData(F("</dd>"));
  }
  if (!GB_StorageHelper.isStoreLogRecordsEnabled()) {
    rawData(F("<dd>"));
    spanTag_RedIfTrue(F("Disabled"), true);
    rawData(F("</dd>"));
  }
  rawData(F("<dd>Stored records: "));
  rawData(GB_StorageHelper.getLogRecordsCount());
  rawData('/');
  rawData(GB_StorageHelper.getLogRecordsCapacity());
  if (GB_StorageHelper.isLogOverflow()) {
    rawData(F(", overflow"));
  }
  rawData(F("</dd>"));

  if (c_isWifiResponseError) {
    return;
  }

  rawData(F("<dt>Other</dt>"));
  rawData(F("<dd>Free memory: "));
  rawData(freeMemory());
  rawData(F(" bytes</dd>"));
  rawData(F("<dd>First startup: "));
  rawData(GB_StorageHelper.getFirstStartupTimeStamp(), false, true);
  rawData(F("</dd>"));

  rawData(F("<dd>"));
  rawData(F("<a href='"));
  rawData(FS(S_URL_GENERAL_OPTIONS_SUMMARY));
  rawData(F("'>View Options Summary</a>"));
  rawData(F("</dd>"));

  rawData(F("</dl>"));

  growboxClockJavaScript(F("growboxTimeStampId"), NULL, F("diffTimeStampId"), F("setClockTimeInput"));

}

/////////////////////////////////////////////////////////////////////
//                             LOG PAGE                            //
/////////////////////////////////////////////////////////////////////
boolean WebServerClass::isSameDay(tmElements_t time1, tmElements_t time2){
  return (time1.Day == time2.Day && time1.Month == time2.Month && time1.Year == time2.Year);
}

void WebServerClass::sendLogPage(const String& getParams) {

  tmElements_t targetDayTm;
  String paramValue;

  // get target Day
  boolean printAllDays = false;
  boolean isTargetDayInParameter = false;
  if (searchHttpParamByName(getParams, F("date"), paramValue)) {
    if (StringUtils::flashStringEquals(paramValue, F("all"))) {
      printAllDays = true;
      isTargetDayInParameter = true;
    }
    else if (paramValue.length() == 10) {
      byte dayInt = paramValue.substring(0, 2).toInt();
      byte monthInt = paramValue.substring(3, 5).toInt();
      word yearInt = paramValue.substring(6, 10).toInt();

      if (dayInt != 0 && monthInt != 0 && yearInt != 0) {
        targetDayTm.Day = dayInt;
        targetDayTm.Month = monthInt;
        targetDayTm.Year = CalendarYrToTm(yearInt);
        isTargetDayInParameter = true;
      }
    }
  }
  if (!isTargetDayInParameter) {
    breakTime(now(), targetDayTm);
  }

  boolean printAll = false, printEvents = false, printWateringEvents = false, printErrors = false, printTemperature = false;
  if (searchHttpParamByName(getParams, F("type"), paramValue)) {
    if (StringUtils::flashStringEquals(paramValue, F("events"))) {
      printEvents = true;
    }
    else if (StringUtils::flashStringEquals(paramValue, F("wateringevents"))) {
      printWateringEvents = true;
    }
    else if (StringUtils::flashStringEquals(paramValue, F("errors"))) {
      printErrors = true;
    }
    else if (StringUtils::flashStringEquals(paramValue, F("temperature"))) {
      printTemperature = true;
    }
    else {
      printAll = true;
    }
  } else {
    printAll = true;
  }

  if (c_isWifiResponseError){
    return;
  }

  rawData(F("<form action='"));
  rawData(FS(S_URL_DAILY_LOG));
  rawData(F("' method='get'>"));

  rawData(F("<select id='typeCombobox' name='type'>"));
  tagOption(F("all"), F("All types"), printAll);
  tagOption(F("events"), F("Events only"), printEvents);
  tagOption(F("wateringevents"), F("Watering Events only"), printWateringEvents);
  tagOption(F("errors"), F("Errors only"), printErrors);
  tagOption(F("temperature"), F("Temperature only"), printTemperature);
  rawData(F("</select>"));

//  String text = StringUtils::flashStringLoad(F("All days  ("));
//  text += GB_StorageHelper.getLogRecordsCount();
//  text += StringUtils::flashStringLoad(F(")"));

  rawData(F("<select id='dateCombobox' name='date'>"));
  //tagOption(F("all"), text, printAllDays);
  // Other will append by java script
  rawData(F("</select>"));
  rawData(F("<input type='submit' value='Show'/>"));
  rawData(F("</form>"));

  LogRecord logRecord, nextLogRecord;
  tmElements_t currentDayTm, nextDayTm;

  boolean isTableTagPrinted = false;
  word currentDayRecordsCount = 0, currentDayPrintableRecordsCount = 0, allPrintableRecordsCount = 0;
  for (word logRecordIndex = 0; logRecordIndex < GB_StorageHelper.getLogRecordsCount(); logRecordIndex++) {

    if (c_isWifiResponseError) {
      return;
    }

    // get current day info
    if (logRecordIndex == 0){
      logRecord = GB_StorageHelper.getLogRecordByIndex(logRecordIndex);
    } else {
      logRecord = nextLogRecord;
    }
    breakTime(logRecord.timeStamp, currentDayTm);

    // is last record in current day?
    boolean isLastRecordInCurrentDay = false;
    if (logRecordIndex == GB_StorageHelper.getLogRecordsCount()-1){
      isLastRecordInCurrentDay = true;
    } else {
      nextLogRecord = GB_StorageHelper.getLogRecordByIndex(logRecordIndex+1);
      breakTime(nextLogRecord.timeStamp, nextDayTm);
      isLastRecordInCurrentDay = !isSameDay(currentDayTm, nextDayTm);
    }

    // increase independent counters
    currentDayRecordsCount++;

    boolean isEvent         = GB_Logger.isEvent(logRecord);
    boolean isWateringEvent = GB_Logger.isWateringEvent(logRecord);
    boolean isError         = GB_Logger.isError(logRecord);
    boolean isTemperature   = GB_Logger.isTemperature(logRecord);

    boolean isPassTypeFilter = (
        (printAll) ||
        (printEvents && isEvent) ||
        (printWateringEvents && isWateringEvent) ||
        (printErrors && isError) ||
        (printTemperature && isTemperature));

    if (isPassTypeFilter) {
      // increase printable counters
      currentDayPrintableRecordsCount++;
      allPrintableRecordsCount++;
    }

    // add date to combo box
    if (isLastRecordInCurrentDay){
      String value = StringUtils::timeStampToString(logRecord.timeStamp, true, false);
      String text = value + StringUtils::flashStringLoad(F("  ("));
      if (!printAll){
        text += currentDayPrintableRecordsCount;
        text +='/';
      }
      text += currentDayRecordsCount;
      text += ')';
      appendOptionToSelectDynamic(F("dateCombobox"), value, text, !printAllDays && isSameDay(currentDayTm, targetDayTm));
    }

    if (isPassTypeFilter && (printAllDays || isSameDay(currentDayTm, targetDayTm))) {
      // Print table row
      if (!isTableTagPrinted) {
        isTableTagPrinted = true;
        rawData(F("<table class='grab align_center'>"));
        rawData(F("<tr>"));
        rawData(F("<th>#</th>"));
        if (printAllDays){
          rawData(F("<th>Day</th>"));
        }
        rawData(F("<th>Time</th><th>Description</th></tr>"));
      }
      rawData(F("<tr>"));

      rawData(F("<td>"));
      if (printAllDays) {
        rawData(allPrintableRecordsCount);
      } else {
        rawData(currentDayPrintableRecordsCount);
      }
      rawData(F("</td>"));

      if (printAllDays){
        rawData(F("<td style='font-weight:bold;'>"));
        if (currentDayPrintableRecordsCount == 1){
          rawData(StringUtils::timeStampToString(logRecord.timeStamp, true, false));
        }
        rawData(F("</td>"));
      }
      rawData(F("<td>"));
      rawData(StringUtils::timeStampToString(logRecord.timeStamp, false, true));
      rawData(F("</td><td style='text-align:left;"));
      if (isError || logRecord.isEmpty()) {
        rawData(F("color:red;"));
      }
      else if (isEvent && (logRecord.data == EVENT_FIRST_START_UP.index || logRecord.data == EVENT_RESTART.index)) { // TODO create check method in Logger.h
        rawData(F("font-weight:bold;"));
      }
      rawData(F("'>"));

      rawData(GB_Logger.getLogRecordDescription(logRecord));
      rawData(GB_Logger.getLogRecordDescriptionSuffix(logRecord, true));
      rawData(F("</td></tr>")); // bug with linker was here https://github.com/arduino/Arduino/issues/1071#issuecomment-19832135
    }

    if (isLastRecordInCurrentDay){ // on day change reset counters
      currentDayRecordsCount = 0;
      currentDayPrintableRecordsCount = 0;
    }
  }

  if (isTableTagPrinted) {
    rawData(F("</table>"));
  }
  else {
    String value = StringUtils::timeStampToString(makeTime(targetDayTm), true, false);
    String text = value + StringUtils::flashStringLoad(F("  ("));
    if (!printAll){
      text += StringUtils::flashStringLoad(F("0/"));
    }
    text += '0';
    text += ')';
    appendOptionToSelectDynamic(F("dateCombobox"), value, text, true); // TODO Maybe will append not in correct position
    rawData(F("<br/>"));
    if (printEvents) {
      rawData(F("Event"));
    } else if (printWateringEvents) {
      rawData(F("Watering event"));
    } else if (printErrors) {
      rawData(F("Error "));
    } else if (printTemperature) {
      rawData(F("Temperature"));
    } else {
      rawData(F("Log"));
    }
    rawData(F(" records not found for "));
    rawData(value);
  }

  String text = StringUtils::flashStringLoad(F("All days  ("));
  if (!printAll){
    text += allPrintableRecordsCount;
    text +='/';
  }
  text += GB_StorageHelper.getLogRecordsCount();
  text += ')';
  appendOptionToSelectDynamic(F("dateCombobox"), F("all"), text, printAllDays);

}

/////////////////////////////////////////////////////////////////////
//                         GENERAL PAGE                           //
/////////////////////////////////////////////////////////////////////

void WebServerClass::sendGeneralOptionsPage_FanParameterRow(const __FlashStringHelper* mode, const __FlashStringHelper* temperature, const __FlashStringHelper* controlNamePrefix, byte fanSpeedValue){

  boolean isOn;
  byte speed, numerator, denominator;
  GB_Controller.unpackFanSpeedValue(fanSpeedValue, isOn, speed, numerator, denominator);

  rawData(F("<tr><td>"));
  rawData(mode);
  rawData(F("</td><td>"));
  rawData(temperature);
  rawData(F("</td><td>"));

  rawData(F("<select name='"));
  rawData(controlNamePrefix);
  rawData(F("Speed'>"));
  tagOption(F("off"), F("Off"), isOn == false);
  tagOption(F("low"), F("Low"), isOn == true && speed == FAN_SPEED_LOW);
  tagOption(F("high"), F("High"),  isOn == true && speed == FAN_SPEED_HIGH);
  rawData(F("</select>"));

  rawData(F("</td><td>"));

  rawData(F("<select name='"));
  rawData(controlNamePrefix);
  rawData(F("Ratio'>"));
  tagOption(F("0"),  F("No ratio"),  numerator == 0 && denominator == 0);
  for (byte index = 1; index < GB_Controller.numeratorDenominatorCombinationsCount(); index++){
    byte l_numerator, l_denominator;
    GB_Controller.getNumeratorDenominatorByIndex(index, l_numerator, l_denominator);

    String value(index);
    String valueDescription (l_numerator * UPDATE_GROWBOX_STATE_DELAY_MINUTES);
    valueDescription += '/';
    valueDescription += l_denominator * UPDATE_GROWBOX_STATE_DELAY_MINUTES;
    tagOption(value, valueDescription, l_numerator == numerator && l_denominator == denominator);
  }
  rawData(F("</select>"));

  rawData(F("</td><td></td></tr>"));
}

void WebServerClass::sendGeneralOptionsPage(const String& getParams) {

  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);
  word dayPeriod = getWordTimePeriodinDay(upTime, downTime); // Can be 24:00
  word nightPeriod = 24 * 60 - dayPeriod; // Can be 00:00

  rawData(F("<fieldset><legend>Clock</legend>"));
  rawData(F("<form action='"));
  rawData(FS(S_URL_GENERAL_OPTIONS));
  rawData(F("' method='post'>"));
  rawData(F("<table class='grab'>"));
  rawData(F("<tr><td>Day mode at</td><td>"));
  tagInputTime(F("turnToDayModeAt"), NULL, upTime, F("g_updDayNightPeriod()")); // see WebServerClass::updateDayNightPeriodJavaScript()
  rawData(F("</td></tr>"));
  rawData(F("<tr><td>Night mode at</td><td>"));
  tagInputTime(F("turnToNightModeAt"), NULL, downTime, F("g_updDayNightPeriod()"));
  rawData(F("</td></tr>"));

  rawData(F("<tr><td>Day/Night period</td><td><span id='dayNightPeriod'>"));
  rawData(StringUtils::wordTimeToString(dayPeriod));
  rawData(F("/"));
  rawData(StringUtils::wordTimeToString(nightPeriod));
  rawData(F("</span></td></tr>"));

  rawData(F("<tr><td>Auto adjust time</td><td>"));
  tagInputNumber(F("autoAdjustTimeStampDelta"), 0, -32768, 32767, GB_Controller.getAutoAdjustClockTimeDelta());
  rawData(F(" sec/day"));
  rawData(F("</td></tr>"));

  rawData(F("</table>"));
  rawData(F("<input type='submit' value='Save'>"));
  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  if (GB_Thermometer.isUseThermometer()) {
    rawData(F("<br/>"));
    if (c_isWifiResponseError)
      return;

    byte normalTemperatueDayMin, normalTemperatueDayMax,
        normalTemperatueNightMin, normalTemperatueNightMax,
        criticalTemperatueMin, criticalTemperatueMax;
    GB_StorageHelper.getTemperatureParameters(
        normalTemperatueDayMin, normalTemperatueDayMax,
        normalTemperatueNightMin, normalTemperatueNightMax,
        criticalTemperatueMin, criticalTemperatueMax);

    rawData(F("<fieldset><legend>Thermometer</legend>"));
    rawData(F("<form action='"));
    rawData(FS(S_URL_GENERAL_OPTIONS));
    rawData(F("' method='post' onSubmit='if (document.getElementById(\"tempDayMin\").value >= document.getElementById(\"tempDayMax\").value "));
    rawData(F("|| document.getElementById(\"tempNightMin\").value >= document.getElementById(\"tempNightMax\").value "));
    rawData(F("|| document.getElementById(\"critTempMin\").value >= document.getElementById(\"critTempMax\").value) { alert(\"Temperature ranges are incorrect\"); return false;}'>"));
    rawData(F("<table>"));

    rawData(F("<tr><td>Day temperature</td><td>"));
    tagInputNumber(F("tempDayMin"), NULL, 1, 50, normalTemperatueDayMin);
    rawData(F(" .. "));
    tagInputNumber(F("tempDayMax"), NULL, 1, 50, normalTemperatueDayMax);
    rawData(F("&deg;C</td></tr>"));

    rawData(F("<tr><td>Night temperature</td><td>"));
    tagInputNumber(F("tempNightMin"), NULL, 1, 50, normalTemperatueNightMin);
    rawData(F(" .. "));
    tagInputNumber(F("tempNightMax"), NULL, 1, 50, normalTemperatueNightMax);
    rawData(F("&deg;C</td></tr>"));

    rawData(F("<tr><td>Critical temperature</td><td>"));
    tagInputNumber(F("critTempMin"), NULL, 1, 50, criticalTemperatueMin);
    rawData(F(" .. "));
    tagInputNumber(F("critTempMax"), NULL, 1, 50, criticalTemperatueMax);
    rawData(F("&deg;C</td></tr>"));

    rawData(F("<tr><td colspan ='2'>"));
    rawData(F("<input type='submit' value='Save'>"));
    rawData(F("</td></tr>"));

    rawData(F("</table>"));
    rawData(F("</form>"));
    rawData(F("</fieldset>"));
  }

  if (GB_Controller.isUseFan()){

    rawData(F("<br/>"));

    byte fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
        fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature, fanSpeedNightHotTemperature;
    GB_StorageHelper.getFanParameters(
        fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
        fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature, fanSpeedNightHotTemperature);

    rawData(F("<fieldset><legend>Fan</legend>"));
    rawData(F("<form action='"));
    rawData(FS(S_URL_GENERAL_OPTIONS));
    rawData(F("' method='post'>"));
    rawData(F("<table>"));

    if (GB_Thermometer.isUseThermometer()) {
      rawData(F("<tr><th>Mode</th><th>Temperature</th><th>Speed</th><th>Ratio</th></tr>"));

      sendGeneralOptionsPage_FanParameterRow(F("Day"), F("Cold"), F("dayCold"), fanSpeedDayColdTemperature);
      sendGeneralOptionsPage_FanParameterRow(F(""), F("Normal"), F("dayNormal"), fanSpeedDayNormalTemperature);
      sendGeneralOptionsPage_FanParameterRow(F(""), F("Hot"), F("dayHot"), fanSpeedDayHotTemperature);

      rawData(F("<tr><td colspan='3'><br/></td></tr>"));

      sendGeneralOptionsPage_FanParameterRow(F("Night"), F("Cold"), F("nightCold"), fanSpeedNightColdTemperature);
      sendGeneralOptionsPage_FanParameterRow(F(""), F("Normal"), F("nightNormal"), fanSpeedNightNormalTemperature);
      sendGeneralOptionsPage_FanParameterRow(F(""), F("Hot"), F("nightHot"), fanSpeedNightHotTemperature);
    }
    else {
      rawData(F("<tr><th>Mode</th><th></th><th>Speed</th><th>Ratio</th></tr>"));
      sendGeneralOptionsPage_FanParameterRow(F("Day"), F(""), F("dayNormal"), fanSpeedDayNormalTemperature);
      sendGeneralOptionsPage_FanParameterRow(F("Night"), F(""), F("nightNormal"), fanSpeedNightNormalTemperature);

    }
    rawData(F("<tr><td colspan='3'>"));
    rawData(F("<input type='submit' value='Save'>"));
    rawData(F("</td></tr>"));

    rawData(F("</table>"));
    rawData(F("</form>"));
    rawData(F("</fieldset>"));
  }

  rawData(F("<br/>"));
  if (c_isWifiResponseError) {
    return;
  }

  rawData(F("<fieldset><legend>Logger</legend>"));

  rawData(F("<form action='"));
  rawData(FS(S_URL_GENERAL_OPTIONS));
  rawData(F("' method='post' id='resetLogForm' onSubmit='return confirm(\"Delete all stored records?\")'>"));
  rawData(F("<input type='hidden' name='resetStoredLog'/>"));
  rawData(F("</form>"));

  rawData(F("<form action='"));
  rawData(FS(S_URL_GENERAL_OPTIONS));
  rawData(F("' method='post'>"));

  rawData(F("<table class='grab'>"));
  rawData(F("<tr><td colspan='2'>"));
  tagCheckbox(F("isStoreLogRecordsEnabled"), F("Enable logger"), GB_StorageHelper.isStoreLogRecordsEnabled());
  rawData(F("<div class='description'>Stored records [<b>"));
  rawData(GB_StorageHelper.getLogRecordsCount());
  rawData('/');
  rawData(GB_StorageHelper.getLogRecordsCapacity());
  if (GB_StorageHelper.isLogOverflow()) {
    rawData(F(", overflow"));
  }
  rawData(F("</b>]"));
  rawData(F("</div></td></tr>"));

  rawData(F("<tr><td>"));
  rawData(F("<input type='submit' value='Save'>"));
  rawData(F("</td><td class='align_right'>"));
  rawData(F("<input form='resetLogForm' type='submit' value='Clear all stored records'/>"));
  rawData(F("</td></tr>"));
  rawData(F("</table>"));

  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  rawData(F("<br/>"));
  rawData(F("<a href='"));
  rawData(FS(S_URL_GENERAL_OPTIONS_SUMMARY));
  rawData(F("'>View Options Summary</a>"));

  updateDayNightPeriodJavaScript();
}

void WebServerClass::sendGeneralOptionsSummaryPage_ModeRow(const __FlashStringHelper* mode, word startTime, word stopTime){

  boolean useThermometer = GB_Thermometer.isUseThermometer();
  boolean useLight  = GB_Controller.isUseLight();
  boolean useFan    = GB_Controller.isUseFan();
  boolean useHeater = GB_Controller.isUseHeater();

  rawData(F("<tr><td class='align_left'>"));
  rawData(mode);
  rawData(F("</td>"));
  if (useThermometer) {
    rawData(F("<td></td>"));
  }
  rawData(F("<td>"));
  if (startTime != 0xFFFF || stopTime != 0xFFFF) {
    rawData(F("<i>"));
    rawData(StringUtils::wordTimeToString(startTime));
    rawData(F(".."));
    rawData(StringUtils::wordTimeToString(stopTime));
    rawData(F("</i>"));
  }
  rawData(F("</td>"));
  if (useLight) {
    rawData(F("<td></td>"));
  }
  if (useFan) {
    rawData(F("<td></td>"));
  }
  if (useHeater) {
    rawData(F("<td></td>"));
  }
  rawData(F("</tr>"));

}

void WebServerClass::sendGeneralOptionsSummaryPage_DataRow(
    const __FlashStringHelper* temperatureRange,
    byte themperatureMin, word themperatureMax,
    boolean isLightOn, byte fanSpeedValue, boolean isHeaterOn){

  boolean useThermometer = GB_Thermometer.isUseThermometer();
  boolean useLight  = GB_Controller.isUseLight();
  boolean useFan    = GB_Controller.isUseFan();
  boolean useHeater = GB_Controller.isUseHeater();

  rawData(F("<tr><td></td>"));
  if (useThermometer) {
    rawData(F("<td class='align_left'>"));
    rawData(temperatureRange);
    rawData(F("</td>"));
  }

  rawData(F("<td>"));
  if (themperatureMin != 0xFF || themperatureMax != 0xFF) {
    if (themperatureMin == 0xFF){
      rawData(F("-&infin;"));
    } else {
      rawData((float)themperatureMin);
    }
    rawData(F(".."));
    if (themperatureMax == 0xFF) {
      rawData(F("&infin;"));
    } else {
      rawData((float)themperatureMax);
    }
    rawData(F("&deg;C"));
  }
  rawData(F("</td>"));

  if (useLight) {
    rawData(F("<td>"));
    rawData(isLightOn ? F("on") : F("off"));
    rawData(F("</td>"));
  }
  if (useFan) {
    rawData(F("<td>"));
    printFanSpeed(fanSpeedValue);
    rawData(F("</td>"));
  }
  if (useHeater) {
    rawData(F("<td>"));
    rawData(isHeaterOn ? F("on") : F("off"));
    rawData(F("</td>"));
  }
  rawData(F("</tr>"));
}

void WebServerClass::sendGeneralOptionsSummaryPage() {

  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  byte normalTemperatueDayMin, normalTemperatueDayMax,
      normalTemperatueNightMin, normalTemperatueNightMax,
      criticalTemperatueMin, criticalTemperatueMax;
  GB_StorageHelper.getTemperatureParameters(
      normalTemperatueDayMin, normalTemperatueDayMax,
      normalTemperatueNightMin, normalTemperatueNightMax,
      criticalTemperatueMin, criticalTemperatueMax);

  byte fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
      fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature, fanSpeedNightHotTemperature;
  GB_StorageHelper.getFanParameters(
      fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
      fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature, fanSpeedNightHotTemperature);

  boolean useThermometer = GB_Thermometer.isUseThermometer();
  boolean useLight  = GB_Controller.isUseLight();
  boolean useFan    = GB_Controller.isUseFan();
  boolean useHeater = GB_Controller.isUseHeater();

  rawData(F("<table class='align_center'>"));

  // Header
  rawData(F("<tr><th>Mode</th>"));
  if (useThermometer) {
    rawData(F("<th>Temperature</th>"));
  }
  rawData(F("<th>Range</th>"));
  if (useLight) {
    rawData(F("<th>Light</th>"));
  }
  if (useFan) {
    rawData(F("<th>Fan</th>"));
  }
  if (useHeater) {
    rawData(F("<th>Heater</th>"));
  }
  rawData(F("</tr>"));

  // Day
  sendGeneralOptionsSummaryPage_ModeRow(F("Day"), upTime, downTime);

  if (useThermometer) {
    sendGeneralOptionsSummaryPage_DataRow(
        F("Critical cold"), 0xFF, criticalTemperatueMin,
        true, GB_Controller.packFanSpeedValue(false), true);

    sendGeneralOptionsSummaryPage_DataRow(
        F("Cold"), criticalTemperatueMin, normalTemperatueDayMin,
        true, fanSpeedDayColdTemperature, true);

    sendGeneralOptionsSummaryPage_DataRow(
        F("Normal"), normalTemperatueDayMin, normalTemperatueDayMax,
        true, fanSpeedDayNormalTemperature, false);

    sendGeneralOptionsSummaryPage_DataRow(
        F("Hot"), normalTemperatueDayMax, criticalTemperatueMax,
        true, fanSpeedDayHotTemperature, false);

    sendGeneralOptionsSummaryPage_DataRow(
        F("Critical hot"), criticalTemperatueMax, 0xFF,
        false, GB_Controller.packFanSpeedValue(true, FAN_SPEED_HIGH), false);
  } else {
      sendGeneralOptionsSummaryPage_DataRow(
          NULL, 0xFF, 0xFF,
          true, fanSpeedDayNormalTemperature, false);
  }
  // Splitter
  sendGeneralOptionsSummaryPage_ModeRow(F("<br/>"));


  // Night
  sendGeneralOptionsSummaryPage_ModeRow(F("Night"), downTime, upTime);
  if (useThermometer) {
    sendGeneralOptionsSummaryPage_DataRow(
        F("Critical cold"), 0xFF, criticalTemperatueMin,
        false, GB_Controller.packFanSpeedValue(false), true);

    sendGeneralOptionsSummaryPage_DataRow(
        F("Cold"), criticalTemperatueMin, normalTemperatueNightMin,
        false, fanSpeedNightColdTemperature, true);

    sendGeneralOptionsSummaryPage_DataRow(
        F("Normal"), normalTemperatueNightMin, normalTemperatueNightMax,
        false, fanSpeedNightNormalTemperature, false);

    sendGeneralOptionsSummaryPage_DataRow(
        F("Hot"), normalTemperatueNightMax, criticalTemperatueMax,
        false, fanSpeedNightHotTemperature, false);

    sendGeneralOptionsSummaryPage_DataRow(
        F("Critical hot"), criticalTemperatueMax, 0xFF,
        false, GB_Controller.packFanSpeedValue(true, FAN_SPEED_HIGH), false);
  } else {
    sendGeneralOptionsSummaryPage_DataRow(
        NULL, 0xFF, 0xFF,
        false, fanSpeedNightNormalTemperature, false);
  }

  rawData(F("</table>"));
}

/////////////////////////////////////////////////////////////////////
//                         WATERING PAGE                           //
/////////////////////////////////////////////////////////////////////

byte WebServerClass::getWateringIndexFromUrl(const String& url) {

  if (!StringUtils::flashStringStartsWith(url, FS(S_URL_WATERING))) {
    return 0xFF;
  }

  String urlSuffix = url.substring(StringUtils::flashStringLength(FS(S_URL_WATERING)));
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

void WebServerClass::sendWateringOptionsPage(const String& url, byte wsIndex) {

  String actionURL;
  actionURL += StringUtils::flashStringLoad(FS(S_URL_WATERING));
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

void WebServerClass::sendHardwareOptionsPage(const String& getParams) {

  rawData(F("<fieldset><legend>Internal hardware</legend>"));

  rawData(F("<form action='"));
  rawData(FS(S_URL_HARDWARE));
  rawData(F("' method='post'>"));

  tagCheckbox(F("useRTC"), F("Use Real-time clock DS1307"), GB_Controller.isUseRTC());
  rawData(F("<div class='description'>Current state [<b>"));
  spanTag_RedIfTrue(
      GB_Controller.isRTCPresent() ? F("connected") : F("not connected"), GB_Controller.isUseRTC() && !GB_Controller.isRTCPresent());
  rawData(F("</b>]</div>"));

  tagCheckbox(F("isEEPROM_AT24C32_Connected"), F("Use external AT24C32 EEPROM"), GB_StorageHelper.isUseExternal_EEPROM_AT24C32());
  rawData(F("<div class='description'>Current state [<b>"));
  spanTag_RedIfTrue(
      EEPROM_AT24C32.isPresent() ? F("connected") : F("not connected"), GB_StorageHelper.isUseExternal_EEPROM_AT24C32() && !EEPROM_AT24C32.isPresent());
  rawData(F("</b>]<br/>On change stored records maybe will lost</div>"));

  tagCheckbox(F("useThermometer"), F("Use Thermometer DS18B20, DS18S20 or DS1822"), GB_Thermometer.isUseThermometer());
  rawData(F("<div class='description'>Current state [<b>"));
  spanTag_RedIfTrue(
      GB_Thermometer.isPresent() ? F("connected") : F("not connected"), GB_Thermometer.isUseThermometer() && !GB_Thermometer.isPresent());
  rawData(F("</b>]</div>"));

  rawData(F("<input type='submit' value='Save'>"));
  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  rawData(F("<br/>"));

  rawData(F("<fieldset><legend>External hardware</legend>"));

  rawData(F("<form action='"));
  rawData(FS(S_URL_HARDWARE));
  rawData(F("' method='post'>"));

  tagCheckbox(F("useLight"), F("Use Light"), GB_Controller.isUseLight());
  rawData(F("<div class='description'>Current state [<b>"));
  rawData(GB_Controller.isLightTurnedOn() ? F("on") : F("off"));
  rawData(F("</b>], mode [<b>"));
  rawData(GB_Controller.isDayInGrowbox() ? F("day") : F("night"));
  rawData(F("</b>]</div>"));

  tagCheckbox(F("useFan"), F("Use Fan"), GB_Controller.isUseFan());
  rawData(F("<div class='description'>Current state [<b>"));
  printFanSpeed(GB_Controller.getFanSpeedValue());
  rawData(F("</b>], temperature [<b>"));
  printTemperatue(GB_Thermometer.getHardwareTemperature());
  rawData(F("</b>]</div>"));

  tagCheckbox(F("useHeater"), F("Use Heater"), GB_Controller.isUseHeater());
  rawData(F("<div class='description'>Current state [<b>"));
  rawData(GB_Controller.isHeaterTurnedOn() ? F("on") : F("off"));
  rawData(F("</b>]</div>"));

  rawData(F("<input type='submit' value='Save'>"));
  rawData(F("</form>"));
  rawData(F("</fieldset>"));

  rawData(F("<br/>"));
  if (c_isWifiResponseError) {
    return;
  }
  boolean isWifiStationMode = GB_StorageHelper.isWifiStationMode();
  rawData(F("<fieldset><legend>Wi-Fi</legend>"));
  rawData(F("<form action='"));
  rawData(FS(S_URL_HARDWARE));
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

void WebServerClass::sendOtherOptionsPage(const String& getParams) {
  //rawData(F("<fieldset><legend>Other</legend>"));  
  rawData(F("<table style='vertical-align:top; border-spacing:0px;'>"));

  rawData(F("<tr><td colspan ='2'>"));
  rawData(F("<a href='"));
  rawData(FS(S_URL_PINMAP));
  rawData(F("'>View pin map</a>"));
  rawData(F("</td></tr>"));

  rawData(F("<tr><td colspan ='2'><br/></td></tr>"));

  rawData(F("<tr><td colspan ='2'>"));
  rawData(F("<a href='"));
  rawData(FS(S_URL_DUMP_INTERNAL));
  rawData(F("'>View internal Arduino dump</a>"));
  rawData(F("</td></tr>"));

  if (EEPROM_AT24C32.isPresent()) {
    rawData(F("<tr><td colspan ='2'>"));
    rawData(F("<a href='"));
    rawData(FS(S_URL_DUMP_AT24C32));
    rawData(F("'>View external AT24C32 dump</a>"));
    rawData(F("</td></tr>"));
  }

  rawData(F("<tr><td colspan ='2'><br/></td></tr>"));

  rawData(F("<tr><td>"));
  rawData(F("<form action='"));
  rawData(FS(S_URL_STATUS));
  rawData(F("' method='post' onSubmit='return confirm(\"Reboot Growbox?\")'>"));
  rawData(F("<input type='hidden' name='rebootController'/><input type='submit' value='Reboot Growbox'>"));
  rawData(F("</form>"));
  rawData(F("</td><td>"));
  rawData(F(" <small>and update page manually</small>"));
  rawData(F("</td></tr>"));

  rawData(F("<tr><td>"));
  rawData(F("<form action='"));
  rawData(FS(S_URL_STATUS));
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

  rawData(F("<form action='"));
  if (isInternal) {
    rawData(FS(S_URL_DUMP_INTERNAL));
  }
  else {
    rawData(FS(S_URL_DUMP_AT24C32));
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

void WebServerClass::sendPinMapPage_TableRow(byte pin, const __FlashStringHelper* description, byte wsIndex){
  rawData(F("<tr><td>"));
  if (pin == 0xFF){
    // Nothing
  } else if (pin < 54){
    rawData(pin);
  } else {
    rawData('A');
    rawData(pin - 54);
  }
  rawData(F("</td><td>"));
  rawData(description);
  if (wsIndex < MAX_WATERING_SYSTEMS_COUNT){
    rawData(wsIndex+1);
  }
  rawData(F("</td>"));
}

void WebServerClass::sendPinMapPage(){
  rawData(F("<table class='grab'>"));
  rawData(F("<tr><th>Pin</th><th>Description</th>"));
  rawData(F("<tr><td>I<sup>2</sup>C</td><td>Real-time clock, external EEPROM</td>"));

  sendPinMapPage_TableRow(LIGHT_PIN, F("Relay: Light"));
  sendPinMapPage_TableRow(FAN_PIN, F("Relay: Fan"));
  sendPinMapPage_TableRow(FAN_SPEED_PIN, F("Relay: Fan speed"));
  sendPinMapPage_TableRow(HEATER_PIN, F("Relay: Heater"));

  sendPinMapPage_TableRow(0xFF, F("<br/>"));

  sendPinMapPage_TableRow(ERROR_PIN, F("Led or Beeper for detect Errors"));
  sendPinMapPage_TableRow(ONE_WIRE_PIN, F("1-Wire: Thermometer"));
  sendPinMapPage_TableRow(BREEZE_PIN, F("Breeze Led (on board)"));

  sendPinMapPage_TableRow(0xFF, F("<br/>"));

  for (int wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    sendPinMapPage_TableRow(WATERING_WET_SENSOR_POWER_PINS[wsIndex], F("Wet sensor power #"), wsIndex);
    sendPinMapPage_TableRow(WATERING_PUMP_PINS[wsIndex], F("Watering pump #"), wsIndex);
  }

  sendPinMapPage_TableRow(0xFF, F("<br/>"));

  sendPinMapPage_TableRow(HARDWARE_BUTTON_RESET_FIRMWARE_PIN, F("Hardware button: Reset firmware"));
  sendPinMapPage_TableRow(HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN, F("Hardware button: Use serial monitor"));

  sendPinMapPage_TableRow(0xFF, F("<br/>"));

  for (int wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    sendPinMapPage_TableRow(WATERING_WET_SENSOR_IN_PINS[wsIndex], F("Wet sensor data #"), wsIndex);
  }
  rawData(F("</table>"));
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

  else if (StringUtils::flashStringEquals(name, F("turnToDayModeAt")) ||
      StringUtils::flashStringEquals(name, F("turnToNightModeAt"))) {
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
  else if (StringUtils::flashStringEquals(name, F("tempDayMin")) ||
      StringUtils::flashStringEquals(name, F("tempDayMax")) ||
      StringUtils::flashStringEquals(name, F("tempNightMin")) ||
      StringUtils::flashStringEquals(name, F("tempNightMax")) ||
      StringUtils::flashStringEquals(name, F("critTempMin")) ||
      StringUtils::flashStringEquals(name, F("critTempMax"))) {
    byte temp = value.toInt();
    if (temp == 0) {
      return false;
    }

    if (StringUtils::flashStringEquals(name, F("tempDayMin"))) {
      GB_StorageHelper.setNormalTemperatueDayMin(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("tempDayMax"))) {
      GB_StorageHelper.setNormalTemperatueDayMax(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("tempNightMin"))) {
      GB_StorageHelper.setNormalTemperatueNightMin(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("tempNightMax"))) {
      GB_StorageHelper.setNormalTemperatueNightMax(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("critTempMin"))) {
      GB_StorageHelper.setCriticalTemperatueMin(temp);
    }
    else if (StringUtils::flashStringEquals(name, F("critTempMax"))) {
      GB_StorageHelper.setCriticalTemperatueMax(temp);
    }

    c_isWifiForceUpdateGrowboxState = true;
  }
  else if (StringUtils::flashStringEquals(name, F("isWetSensorConnected")) ||
      StringUtils::flashStringEquals(name, F("isWaterPumpConnected")) ||
      StringUtils::flashStringEquals(name, F("useWetSensorForWatering")) ||
      StringUtils::flashStringEquals(name, F("skipNextWatering"))) {

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
  else if (StringUtils::flashStringEquals(name, F("inAirValue")) ||
      StringUtils::flashStringEquals(name, F("veryDryValue")) ||
      StringUtils::flashStringEquals(name, F("dryValue")) ||
      StringUtils::flashStringEquals(name, F("normalValue")) ||
      StringUtils::flashStringEquals(name, F("wetValue")) ||
      StringUtils::flashStringEquals(name, F("veryWetValue"))) {

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
  else if (StringUtils::flashStringEquals(name, F("dryWateringDuration")) ||
      StringUtils::flashStringEquals(name, F("veryDryWateringDuration"))) {

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
    GB_Controller.setClockTime(newTimeStamp + WEB_SERVER_AVERAGE_PAGE_LOAD_TIME_SEC);

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
    GB_Thermometer.setUseThermometer(boolValue);

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
  else if (StringUtils::flashStringEquals(name, F("useHeater"))) {
    if (value.length() != 1) {
      return false;
    }
    boolean boolValue = (value[0] == '1');
    GB_Controller.setUseHeater(boolValue);

    c_isWifiForceUpdateGrowboxState = true;

  }
  else if (StringUtils::flashStringEquals(name, F("dayColdSpeed")) ||
      StringUtils::flashStringEquals(name, F("dayNormalSpeed")) ||
      StringUtils::flashStringEquals(name, F("dayHotSpeed")) ||
      StringUtils::flashStringEquals(name, F("nightColdSpeed")) ||
      StringUtils::flashStringEquals(name, F("nightNormalSpeed")) ||
      StringUtils::flashStringEquals(name, F("nightHotSpeed"))) {
    byte isOn, speed;
    if (StringUtils::flashStringEquals(value, F("off"))) {
      isOn = false;
      speed = RELAY_OFF;  // Compiler warning
    }
    else if (StringUtils::flashStringEquals(value, F("low"))) {
      isOn = true;
      speed = FAN_SPEED_LOW;
    }
    else if (StringUtils::flashStringEquals(value, F("high"))) {
      isOn = true;
      speed = FAN_SPEED_HIGH;
    }
    else  {
      return false;
    }

    byte fanSpeedValue;
    byte fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
         fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature,fanSpeedNightHotTemperature;
    GB_StorageHelper.getFanParameters(
        fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
        fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature, fanSpeedNightHotTemperature);

    if (StringUtils::flashStringEquals(name, F("dayColdSpeed"))) {
      fanSpeedValue = fanSpeedDayColdTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("dayNormalSpeed"))){
      fanSpeedValue = fanSpeedDayNormalTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("dayHotSpeed"))){
      fanSpeedValue = fanSpeedDayHotTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("nightColdSpeed"))){
      fanSpeedValue = fanSpeedNightColdTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("nightNormalSpeed"))){
      fanSpeedValue = fanSpeedNightNormalTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("nightHotSpeed"))){
      fanSpeedValue = fanSpeedNightHotTemperature;
    }
    else {
      return false;
    }

    boolean isOn_temp; byte speed_temp;
    byte numerator, denominator;
    GB_Controller.unpackFanSpeedValue(fanSpeedValue, isOn_temp, speed_temp, numerator, denominator);
    fanSpeedValue = GB_Controller.packFanSpeedValue(isOn, speed, numerator, denominator);

    if (StringUtils::flashStringEquals(name, F("dayColdSpeed"))) {
      GB_StorageHelper.setFanSpeedDayColdTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("dayNormalSpeed"))){
      GB_StorageHelper.setFanSpeedDayNormalTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("dayHotSpeed"))){
      GB_StorageHelper.setFanSpeedDayHotTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("nightColdSpeed"))){
      GB_StorageHelper.setFanSpeedNightColdTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("nightNormalSpeed"))){
      GB_StorageHelper.setFanSpeedNightNormalTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("nightHotSpeed"))){
      GB_StorageHelper.setFanSpeedNightHotTemperature(fanSpeedValue);
    }
    else {
      return false;
    }
    c_isWifiForceUpdateGrowboxState = true;
  }
  else if (StringUtils::flashStringEquals(name, F("dayColdRatio")) ||
      StringUtils::flashStringEquals(name, F("dayNormalRatio")) ||
      StringUtils::flashStringEquals(name, F("dayHotRatio")) ||
      StringUtils::flashStringEquals(name, F("nightColdRatio")) ||
      StringUtils::flashStringEquals(name, F("nightNormalRatio")) ||
      StringUtils::flashStringEquals(name, F("nightHotRatio"))) {
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
    byte numerator, denominator;
    GB_Controller.getNumeratorDenominatorByIndex(intValue, numerator, denominator);

    byte fanSpeedValue;
    byte fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
         fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature,fanSpeedNightHotTemperature;
    GB_StorageHelper.getFanParameters(
        fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
        fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature, fanSpeedNightHotTemperature);

    if (StringUtils::flashStringEquals(name, F("dayColdRatio"))) {
      fanSpeedValue = fanSpeedDayColdTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("dayNormalRatio"))){
      fanSpeedValue = fanSpeedDayNormalTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("dayHotRatio"))){
      fanSpeedValue = fanSpeedDayHotTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("nightColdRatio"))){
      fanSpeedValue = fanSpeedNightColdTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("nightNormalRatio"))){
      fanSpeedValue = fanSpeedNightNormalTemperature;
    }
    else if (StringUtils::flashStringEquals(name, F("nightHotRatio"))){
      fanSpeedValue = fanSpeedNightHotTemperature;
    }
    else {
      return false;
    }

    boolean isOn; byte speed;
    byte numerator_temp, denominator_temp;
    GB_Controller.unpackFanSpeedValue(fanSpeedValue, isOn, speed, numerator_temp, denominator_temp);
    fanSpeedValue = GB_Controller.packFanSpeedValue(isOn, speed, numerator, denominator);

    if (StringUtils::flashStringEquals(name, F("dayColdRatio"))) {
      GB_StorageHelper.setFanSpeedDayColdTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("dayNormalRatio"))){
      GB_StorageHelper.setFanSpeedDayNormalTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("dayHotRatio"))){
      GB_StorageHelper.setFanSpeedDayHotTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("nightColdRatio"))){
      GB_StorageHelper.setFanSpeedNightColdTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("nightNormalRatio"))){
      GB_StorageHelper.setFanSpeedNightNormalTemperature(fanSpeedValue);
    }
    else if (StringUtils::flashStringEquals(name, F("nightHotRatio"))){
      GB_StorageHelper.setFanSpeedNightHotTemperature(fanSpeedValue);
    }
    else {
      return false;
    }
    c_isWifiForceUpdateGrowboxState = true;
  }
  else {
    return false;
  }

  return true;
}
