#ifndef WebServer_h
#define WebServer_h

#include "Global.h"

/////////////////////////////////////////////////////////////////////
//                           HTML CONSTS                           //
/////////////////////////////////////////////////////////////////////

const char S_url_root[] PROGMEM  = "/";
const char S_url_log[] PROGMEM  = "/log";
const char S_url_watering[] PROGMEM  = "/watering";
const char S_url_general [] PROGMEM  = "/general";
const char S_url_hardware [] PROGMEM  = "/hardware";
const char S_url_other [] PROGMEM  = "/other";
const char S_url_dump_internal[] PROGMEM  = "/other/dump_internal";
const char S_url_dump_AT24C32[] PROGMEM  = "/other/dump_AT24C32";


class WebServerClass{
private:  
  byte c_wifiPortDescriptor;
  byte c_isWifiResponseError;
  byte c_isWifiForceUpdateGrowboxState;

public:  
  void init();
  void update();

public:  
  boolean handleSerialEvent();
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
  template <class T> void rawData(T data){
    String str(data);
    rawData(str);
  }

  void tagButton(const __FlashStringHelper* url, const __FlashStringHelper* text, boolean isSelected);
  void tagCheckbox(const __FlashStringHelper* name, const __FlashStringHelper* text, boolean isSelected);
  void tagRadioButton(const __FlashStringHelper* name, const __FlashStringHelper* text, const __FlashStringHelper* value, boolean isSelected);
  void tagInputNumber(const __FlashStringHelper* name, const __FlashStringHelper* text, word minValue, word maxValue, word value);
  void tagInputTime(const __FlashStringHelper* name, const __FlashStringHelper* text, word value);
  word getTimeFromInput(const String& value);

  void tagOption(const String& value, const String& text, boolean isSelected,  boolean isDisabled = false);
  void tagOption(const __FlashStringHelper* value, const __FlashStringHelper* text, boolean isSelected,  boolean isDisabled = false);

  void appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const String& value, const String& text, boolean isSelected);
  void appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const String& text, boolean isSelected);
  void appendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const __FlashStringHelper* text, boolean isSelected);

  void growboxClockJavaScript(const __FlashStringHelper* growboxTimeStampId = 0, const __FlashStringHelper* browserTimeStampId = 0, const __FlashStringHelper* diffTimeStampId = 0);
  void spanTag_RedIfTrue(const __FlashStringHelper* text, boolean isRed);
  
  /////////////////////////////////////////////////////////////////////
  //                     COMMON FOR ALL PAGES                        //
  /////////////////////////////////////////////////////////////////////

  void httpProcessGet(const String &input, const String &getParams);

  /////////////////////////////////////////////////////////////////////
  //                          STATUS PAGE                            //
  /////////////////////////////////////////////////////////////////////

  void sendStatusPage();

  /////////////////////////////////////////////////////////////////////
  //                             LOG PAGE                            //
  /////////////////////////////////////////////////////////////////////

  void sendLogPage(const String& getParams);
  
  /////////////////////////////////////////////////////////////////////
  //                          GENERAL PAGE                           //
  /////////////////////////////////////////////////////////////////////
  
  void sendGeneralPage(const String& getParams);
 
  /////////////////////////////////////////////////////////////////////
  //                         WATERING PAGE                           //
  /////////////////////////////////////////////////////////////////////
  
  byte getWateringIndexFromUrl(const String& url);
  void sendWateringPage(const String& url, byte wsIndex);
 
  /////////////////////////////////////////////////////////////////////
  //                        HARDWARE PAGES                           //
  /////////////////////////////////////////////////////////////////////

  void sendHardwarePage(const String& getParams);
  void sendStorageDumpPage(const String& getParams, boolean isInternal);
  void sendOtherPage(const String& getParams);

  /////////////////////////////////////////////////////////////////////
  //                          POST HANDLING                          //
  /////////////////////////////////////////////////////////////////////

  String applyPostParams(const String& url, const String& postParams);
  boolean applyPostParam(const String& url, const String& name, const String& value);  

  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////

  template <class T> void showWebMessage(T str, boolean newLine = true){
    if (g_useSerialMonitor){
      Serial.print(F("WEB> "));
      Serial.print(str);
      if (newLine){  
        Serial.println();
      }      
    }
  }

};

extern WebServerClass GB_WebServer;

#endif


