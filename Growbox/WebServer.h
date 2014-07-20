#ifndef WebServer_h
#define WebServer_h

#include "Global.h"

/////////////////////////////////////////////////////////////////////
//                           HTML CONSTS                           //
/////////////////////////////////////////////////////////////////////


enum HTTP_TAG {
  HTTP_TAG_OPEN, HTTP_TAG_CLOSED, HTTP_TAG_SINGLE
};

const char S_hr[] PROGMEM  = "hr";
const char S_br[] PROGMEM  = "br";
const char S_table[] PROGMEM  = "table";
const char S_tr[] PROGMEM  = "tr";
const char S_td[] PROGMEM  = "td";
const char S_pre[] PROGMEM  = "pre";
const char S_html[] PROGMEM  = "html";
const char S_h1[] PROGMEM  = "h1";

const char S_url_root[] PROGMEM  = "/";
//const char S_url_pins[] PROGMEM  = "/pins";
const char S_url_log[] PROGMEM  = "/log";
const char S_url_configuration[] PROGMEM  = "/conf";
const char S_url_storage[] PROGMEM  = "/storage";

class WebServerClass{
private:  

  byte c_wifiPortDescriptor;
  byte c_isWifiResponseError;

public:  

  void handleSerialEvent();

private:

  /////////////////////////////////////////////////////////////////////
  //                               HTTP                              //
  /////////////////////////////////////////////////////////////////////

  void sendHttpNotFound();
  void sendHttpRedirect(const String &url);

  void sendHttpPageHeader();
  void sendHttpPageComplete();
  void sendHttpPageBody(const String &input, const String &getParams);

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

  void sendTagButton(const __FlashStringHelper* buttonUrl, const __FlashStringHelper* buttonTitle, boolean isSelected);
  void sendTagCheckbox(const __FlashStringHelper* name, const __FlashStringHelper* text, boolean isSelected, const __FlashStringHelper* extra = 0);
  void sendAppendOptionToSelectDynamic(const __FlashStringHelper* selectId, const __FlashStringHelper* value, const String& optionText, boolean isSelected);

  template <class T> void sendTag(T tagName, HTTP_TAG type){
    sendRawData('<');
    if (type == HTTP_TAG_CLOSED){
      sendRawData('/');
    }
    sendRawData(tagName);
    if (type == HTTP_TAG_SINGLE){
      sendRawData('/');
    }
    sendRawData('>');
  }

  /////////////////////////////////////////////////////////////////////
  //                          STATUS PAGE                            //
  /////////////////////////////////////////////////////////////////////

  void sendStatusPage();

  /////////////////////////////////////////////////////////////////////
  //                      CONFIGURATION PAGE                         //
  /////////////////////////////////////////////////////////////////////

  void sendConfigurationPage(const String& getParams);

  /////////////////////////////////////////////////////////////////////
  //                             LOG PAGE                            //
  /////////////////////////////////////////////////////////////////////

  void sendLogPageHeader();
  void sendLogPage(const String& getParams);

  /////////////////////////////////////////////////////////////////////
  //                          OTHER PAGES                            //
  /////////////////////////////////////////////////////////////////////
  // TODO garbage?
  void printStorage(word address, byte sizeOf);

  void sendStorageDump();

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
























