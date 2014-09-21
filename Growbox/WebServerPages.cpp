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

  boolean isGeneralPage = StringUtils::flashStringEquals(url, FS(S_URL_GENERAL_OPTIONS));
  boolean isWateringPage = (wsIndex != 0xFF);
  boolean isHardwarePage = StringUtils::flashStringEquals(url, FS(S_URL_HARDWARE));
  boolean isDumpInternal = StringUtils::flashStringEquals(url, FS(S_URL_DUMP_INTERNAL));
  boolean isDumpAT24C32 = StringUtils::flashStringEquals(url, FS(S_URL_DUMP_AT24C32));
  boolean isOtherPage = StringUtils::flashStringEquals(url, FS(S_URL_OTHER_PAGE));

  boolean isConfigurationPage = (isGeneralPage || isWateringPage || isHardwarePage || isDumpInternal || isDumpAT24C32 || isOtherPage);

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
      rawData(F(".align_right{float:right;text-align:right;}"));
      rawData(F(".align_center{float:center;text-align:center;}"));
      rawData(F(".description{font-size:small;margin-left:20px;margin-bottom:5px;}"));
    }
    rawData(F("</style>"));
  }

  rawData(F("</head>"));

  rawData(F("<body>"));
  rawData(F("<h1>Growbox</h1>"));
  if (isCriticalErrorOnStatusPage()){
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
  tagOption(FS(S_URL_GENERAL_OPTIONS), F("General options"), isGeneralPage);
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
  else if (isGeneralPage) {
    sendGeneralOptionsPage(getParams);
  }
  else if (isWateringPage) {
    sendWateringOptionsPage(url, wsIndex);
  }
  else if (isHardwarePage) {
    sendHardwareOptionsPage(getParams);
  }
  else if (isDumpInternal || isDumpAT24C32) {
    sendStorageDumpPage(getParams, isDumpInternal);
  }
  else if (isOtherPage) {
    sendOtherOptionsPage(getParams);
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
      (GB_StorageHelper.isUseThermometer() && !GB_Thermometer.isPresent())
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
  rawData(isDayInGrowbox ? F("Day") : F("Night"));
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
    rawData(GB_Controller.isLightTurnedOn() ? F("Enabled") : F("Disabled"));
    rawData(F("</dd>"));
  }

  if (GB_Controller.isUseFan()) {
    rawData(F("<dd>Fan: "));
    boolean isFanEnabled = GB_Controller.isFanTurnedOn();
    rawData(isFanEnabled ? ((GB_Controller.getFanSpeed() == FAN_SPEED_MIN) ? F("Min speed") : F("Max speed")) : F("Disabled"));
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

  if (GB_StorageHelper.isUseThermometer()) {
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
      rawData(F("<dd>Wet sensor: "));
      WateringEvent* currentStatus = GB_Watering.getCurrentWetSensorStatus(wsIndex);
      spanTag_RedIfTrue(currentStatus->shortDescription, currentStatus != &WATERING_EVENT_WET_SENSOR_NORMAL);

      byte value = GB_Watering.getCurrentWetSensorValue(wsIndex);
      if (!GB_Watering.isWetSensorValueReserved(value)) {
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

  rawData(F("<dt>Configuration</dt>"));
  rawData(F("<dd>Day mode at: "));
  rawData(StringUtils::wordTimeToString(upTime));
  rawData(F("</dd><dd>Night mode at: "));
  rawData(StringUtils::wordTimeToString(downTime));
  rawData(F("</dd>"));

  if (GB_StorageHelper.isUseThermometer()) {
    byte normalTemperatueDayMin, normalTemperatueDayMax,
        normalTemperatueNightMin, normalTemperatueNightMax,
        criticalTemperatue;
    GB_StorageHelper.getTemperatureParameters(
        normalTemperatueDayMin, normalTemperatueDayMax,
        normalTemperatueNightMin, normalTemperatueNightMax,
        criticalTemperatue);

    rawData(F("<dd>Normal Day temperature: "));
    printTemperatueRange((float)normalTemperatueDayMin, (float)normalTemperatueDayMax);
    rawData(F("</dd><dd>Normal Night temperature: "));
    printTemperatueRange((float)normalTemperatueNightMin, (float)normalTemperatueNightMax);
    rawData(F("</dd><dd>Critical temperature: "));
    printTemperatue((float)criticalTemperatue);
    rawData(F("</dd>"));
  }

  rawData(F("<dt>Other</dt>"));
  rawData(F("<dd>Free memory: "));
  rawData(freeMemory());
  rawData(F(" bytes</dd>"));
  rawData(F("<dd>First startup: "));
  rawData(GB_StorageHelper.getFirstStartupTimeStamp(), false, true);
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

  rawData(F("<select id='dateCombobox' name='date'>"));
  tagOption(F("all"), F("All days"), printAllDays);
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
}

/////////////////////////////////////////////////////////////////////
//                         GENERAL PAGE                           //
/////////////////////////////////////////////////////////////////////

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

  rawData(F("<br/>"));
  if (c_isWifiResponseError)
    return;

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

  if (GB_StorageHelper.isUseThermometer()) {

    rawData(F("<br/>"));
    if (c_isWifiResponseError)
      return;

    byte normalTemperatueDayMin, normalTemperatueDayMax,
        normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue;
    GB_StorageHelper.getTemperatureParameters(normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue);

    rawData(F("<fieldset><legend>Thermometer</legend>"));
    rawData(F("<form action='"));
    rawData(FS(S_URL_GENERAL_OPTIONS));
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

  updateDayNightPeriodJavaScript();
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

  rawData(F("<fieldset><legend>General</legend>"));

  rawData(F("<form action='"));
  rawData(FS(S_URL_HARDWARE));
  rawData(F("' method='post'>"));

  tagCheckbox(F("useLight"), F("Use Light"), GB_Controller.isUseLight());
  rawData(F("<div class='description'>Current state [<b>"));
  rawData(GB_Controller.isLightTurnedOn() ? F("Enabled") : F("Disabled"));
  rawData(F("</b>], mode [<b>"));
  rawData(GB_Controller.isDayInGrowbox() ? F("Day") : F("Night"));
  rawData(F("</b>]</div>"));

  tagCheckbox(F("useFan"), F("Use Fan"), GB_Controller.isUseFan());
  rawData(F("<div class='description'>Current state [<b>"));
  rawData(GB_Controller.isFanTurnedOn() ? (GB_Controller.getFanSpeed() == FAN_SPEED_MAX ? F("Max speed") : F("Min speed")) : F("Disabled"));
  rawData(F("</b>], temperature [<b>"));
  printTemperatue(GB_Thermometer.getTemperature());
  rawData(F("</b>]</div>"));

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
