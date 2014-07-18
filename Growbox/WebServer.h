#ifndef WebServer_h
#define WebServer_h

#include <MemoryFree.h>

#include "Global.h"
#include "StorageHelper.h" 
#include "Thermometer.h" 

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

  void showWebMessage(const __FlashStringHelper* str, boolean newLine = true);

  /////////////////////////////////////////////////////////////////////
  //                           HTTP PROTOCOL                         //
  /////////////////////////////////////////////////////////////////////

  void sendHttpNotFound();
  void sendHttpRedirect(const String &url);

  void sendHttpPageHeader();
  void sendHttpPageComplete();
  void sendHttpPageBody(const String &input);

  /////////////////////////////////////////////////////////////////////
  //                    HTML SUPPLEMENTAL COMMANDS                   //
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
  //                          HTML PAGE PARTS                        //
  /////////////////////////////////////////////////////////////////////

  void sendBriefStatus();

  void sendFreeMemory();
  void sendBootStatus();
  void sendTimeStatus();
  void sendTemperatureStatus();
  void sendPinsStatus();

  void sendConfigurationForms();
  String applyPostParams(String& postParams);
  boolean applyPostParam(String& postParam);  

  void sendFullLog(boolean printEvents, boolean printErrors, boolean printTemperature);

  // TODO garbage?
  void printStorage(word address, byte sizeOf);

  void sendStorageDump();

};

extern WebServerClass GB_WebServer;

#endif









