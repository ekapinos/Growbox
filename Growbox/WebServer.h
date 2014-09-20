#ifndef WebServer_h
#define WebServer_h

#include "Global.h"

/////////////////////////////////////////////////////////////////////
//                           HTML CONSTS                           //
/////////////////////////////////////////////////////////////////////

const char S_URL_STATUS[] PROGMEM = "/";
const char S_URL_DAILY_LOG[] PROGMEM = "/log";
const char S_URL_GENERAL_OPTIONS[] PROGMEM = "/general";
const char S_URL_WATERING[] PROGMEM = "/watering";
const char S_URL_HARDWARE[] PROGMEM = "/hardware";
const char S_URL_OTHER_PAGE[] PROGMEM = "/other";
const char S_URL_DUMP_INTERNAL[] PROGMEM = "/other/dump_internal";
const char S_URL_DUMP_AT24C32[] PROGMEM = "/other/dump_AT24C32";

class WebServerClass{
private:
  byte c_wifiPortDescriptor;
  byte c_isWifiResponseError;
  byte c_isWifiForceUpdateGrowboxState;

public:
  void init();
  void update();

public:
  boolean handleSerialWiFiEvent();
  boolean handleSerialMonitorEvent();

  /////////////////////////////////////////////////////////////////////
  //                               HTTP                              //
  /////////////////////////////////////////////////////////////////////
private:
  void httpNotFound();
  void httpRedirect(const String &url);

  void httpPageHeader();
  void httpPageComplete();

  /////////////////////////////////////////////////////////////////////
  //                         HTTP PARAMETERS                         //
  /////////////////////////////////////////////////////////////////////

  String encodeHttpString(const String& dirtyValue);
  boolean getHttpParamByIndex(const String& params, const word index, String& name, String& value);
  boolean searchHttpParamByName(const String& params, const __FlashStringHelper* targetName, String& targetValue);

  /////////////////////////////////////////////////////////////////////
  //                               HTML                              //
  /////////////////////////////////////////////////////////////////////

  void rawData(const __FlashStringHelper* data);
  void rawData(const String &data);
  void rawData(float data);
  void rawData(time_t data, boolean interpretateAsULong = false, boolean forceShowZeroTimeStamp = false);
  template <class T> void rawData(T data) {
    String str(data);
    rawData(str);
  }

  void tagButton(const __FlashStringHelper* url, const __FlashStringHelper* text, boolean isSelected);
  void tagCheckbox(const __FlashStringHelper* name, const __FlashStringHelper* text, boolean isSelected);
  void tagRadioButton(const __FlashStringHelper* name, const __FlashStringHelper* text, const __FlashStringHelper* value, boolean isSelected);
  void tagInputNumber(const __FlashStringHelper* name, const __FlashStringHelper* text, long minValue, long maxValue, long value);
  void tagInputTime(const __FlashStringHelper* name, const __FlashStringHelper* text, word value, const __FlashStringHelper* onChange = NULL);
  word getTimeFromInput(const String& value);

  void tagOption(const String& value, const String& text, boolean isSelected, boolean isDisabled = false);
  void tagOption(const __FlashStringHelper* value, const __FlashStringHelper* text, boolean isSelected, boolean isDisabled = false);

  void appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const String& value, const String& text, boolean isSelected);
  void appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const String& text, boolean isSelected);
  void appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const __FlashStringHelper* text, boolean isSelected);

  void growboxClockJavaScript(const __FlashStringHelper* growboxTimeStampId = NULL, const __FlashStringHelper* browserTimeStampId = NULL, const __FlashStringHelper* diffTimeStampId = NULL, const __FlashStringHelper* setClockTimeHiddenInputId = NULL);
  void spanTag_RedIfTrue(const __FlashStringHelper* text, boolean isRed);
  void printTemperatue(float t);
  void printTemperatueRange(float t1, float t2);

  /////////////////////////////////////////////////////////////////////
  //                      COMMON FOR ALL PAGES                       //
  /////////////////////////////////////////////////////////////////////

  void httpProcessGet(const String &input, const String &getParams);

  /////////////////////////////////////////////////////////////////////
  //                          STATUS PAGE                            //
  /////////////////////////////////////////////////////////////////////
  boolean isCriticalErrorOnStatusPage();
  word getWordTimePeriodinDay(word start, word stop);
  void sendStatusPage();

  /////////////////////////////////////////////////////////////////////
  //                            LOG PAGE                             //
  /////////////////////////////////////////////////////////////////////

  boolean isSameDay(tmElements_t time1, tmElements_t time2);
  void sendLogPage(const String& getParams);

  /////////////////////////////////////////////////////////////////////
  //                          GENERAL PAGE                           //
  /////////////////////////////////////////////////////////////////////

  void updateDayNightPeriodJavaScript();
  void sendGeneralOptionsPage(const String& getParams);

  /////////////////////////////////////////////////////////////////////
  //                         WATERING PAGE                           //
  /////////////////////////////////////////////////////////////////////

  byte getWateringIndexFromUrl(const String& url);
  void sendWateringOptionsPage(const String& url, byte wsIndex);

  /////////////////////////////////////////////////////////////////////
  //                        HARDWARE PAGES                           //
  /////////////////////////////////////////////////////////////////////

  void sendHardwareOptionsPage(const String& getParams);
  void sendStorageDumpPage(const String& getParams, boolean isInternal);
  void sendOtherOptionsPage(const String& getParams);

  /////////////////////////////////////////////////////////////////////
  //                          POST HANDLING                          //
  /////////////////////////////////////////////////////////////////////

  String applyPostParams(const String& url, const String& postParams);
  boolean applyPostParam(const String& url, const String& name, const String& value);

  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////

  template <class T> void showWebMessage(T str, boolean newLine = true) {
    if (g_useSerialMonitor) {
      Serial.print(F("WEB> "));
      Serial.print(str);
      if (newLine) {
        Serial.println();
      }
    }
  }

};

extern WebServerClass GB_WebServer;

#endif

