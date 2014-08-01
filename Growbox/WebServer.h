#ifndef WebServer_h
#define WebServer_h

#include "Global.h"

/////////////////////////////////////////////////////////////////////
//                           HTML CONSTS                           //
/////////////////////////////////////////////////////////////////////

const char S_url_root[] PROGMEM  = "/";
//const char S_url_pins[] PROGMEM  = "/pins";
const char S_url_log[] PROGMEM  = "/log";
const char S_url_watering[] PROGMEM  = "/watering";
const char S_url_configuration[] PROGMEM  = "/conf";
const char S_url_storage[] PROGMEM  = "/conf/storage";

class WebServerClass{
private:  

  byte c_wifiPortDescriptor;
  byte c_isWifiResponseError;
  byte c_isWifiForceUpdateGrowboxState;
public:  

  boolean handleSerialEvent();

private:

  /////////////////////////////////////////////////////////////////////
  //                               HTTP                              //
  /////////////////////////////////////////////////////////////////////

  void sendHttpNotFound();
  void sendHttpRedirect(const String &url);

  void sendHttpPageHeader();
  void sendHttpPageComplete();

  /////////////////////////////////////////////////////////////////////
  //                         HTTP PARAMETERS                         //
  /////////////////////////////////////////////////////////////////////

  String encodeHttpString(const String& dirtyValue);
  boolean getHttpParamByIndex(const String& params, const word index, String& name, String& value);
  boolean searchHttpParamByName(const String& params, const __FlashStringHelper* targetName, String& targetValue);

  /////////////////////////////////////////////////////////////////////
  //                               HTML                              //
  /////////////////////////////////////////////////////////////////////

  void sendRawData(const __FlashStringHelper* data);
  void sendRawData(const String &data);
  void sendRawData(float data);
  void sendRawData(time_t data);
  template <class T> void sendRawData(T data){
    String str(data);
    sendRawData(str);
  }

  void sendTagButton(const __FlashStringHelper* url, const __FlashStringHelper* text, boolean isSelected);
  void sendTagCheckbox(const __FlashStringHelper* name, const __FlashStringHelper* text, boolean isSelected);
  void sendTagRadioButton(const __FlashStringHelper* name, const __FlashStringHelper* text, const __FlashStringHelper* value, boolean isSelected);
  void sendAppendOptionToSelectDynamic(const __FlashStringHelper* selectId, const String& value, const String& text, boolean isSelected);
  void sendAppendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const String& text, boolean isSelected);
  void sendAppendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const __FlashStringHelper* text, boolean isSelected);

  /////////////////////////////////////////////////////////////////////
  //                        COMMON FOR PAGES                         //
  /////////////////////////////////////////////////////////////////////
  
  void sendHttpPageBody(const String &input, const String &getParams);

  /////////////////////////////////////////////////////////////////////
  //                          STATUS PAGE                            //
  /////////////////////////////////////////////////////////////////////

  void sendStatusPage();

  /////////////////////////////////////////////////////////////////////
  //                         WATERING PAGE                           //
  /////////////////////////////////////////////////////////////////////

  void sendWateringPage();
  
  /////////////////////////////////////////////////////////////////////
  //                      CONFIGURATION PAGE                         //
  /////////////////////////////////////////////////////////////////////

  void sendConfigurationPage(const String& getParams);
  void sendStorageDumpPage(const String& getParams);

  /////////////////////////////////////////////////////////////////////
  //                             LOG PAGE                            //
  /////////////////////////////////////////////////////////////////////

  void sendLogPage(const String& getParams);

  /////////////////////////////////////////////////////////////////////
  //                          OTHER PAGES                            //
  /////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////
  //                          POST HANDLING                          //
  /////////////////////////////////////////////////////////////////////

  String applyPostParams(const String& url, const String& postParams);
  boolean applyPostParam(const String& name, const String& value);  

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



























