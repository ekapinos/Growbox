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

const char S_url[] PROGMEM  = "/";
const char S_url_log[] PROGMEM  = "/log";
const char S_url_conf[] PROGMEM  = "/conf";
const char S_url_storage[] PROGMEM  = "/storage";

class WebServerClass{
private:  

  byte c_wifiPortDescriptor;
  byte c_isWifiResponseError;

public:  

  void handleSerialEvent();

private:

  static void showWebMessage(const __FlashStringHelper* str, boolean newLine = true);

  /////////////////////////////////////////////////////////////////////
  //                           HTTP PROTOCOL                         //
  /////////////////////////////////////////////////////////////////////

  static void sendHttpNotFound(const byte wifiPortDescriptor);
  static void sendHTTPRedirect(const byte &wifiPortDescriptor, const __FlashStringHelper* data);
  static void sendHttpOK_Header(const byte wifiPortDescriptor);
  static void sendHttpOK_PageComplete(const byte &wifiPortDescriptor);

  /////////////////////////////////////////////////////////////////////
  //                  HTTP 200 OK SUPPLEMENTAL COMMANDS              //
  /////////////////////////////////////////////////////////////////////

  void sendData(const __FlashStringHelper* data);
  void sendData(const String &data);
  void sendData(int data);
  void sendData(word data);
  void sendData(char data);
  void sendData(float data);
  void sendData(time_t data);

  void sendDataLn();
  void sendTagButton(const __FlashStringHelper* url, const __FlashStringHelper* name);
  void sendTag_Begin(HTTP_TAG type);
  void sendTag_End(HTTP_TAG type);
  void sendTag(const __FlashStringHelper* pname, HTTP_TAG type);
  void sendTag(const char tag, HTTP_TAG type);

  void generateHttpResponsePage(const String &input);

  void sendBriefStatus();

  void sendConfigurationControls();
  void sendFreeMemory();
  void sendBootStatus();
  void sendTimeStatus();
  void sendTemperatureStatus();
  void sendPinsStatus();


  void printSendFullLog(boolean printEvents, boolean printErrors, boolean printTemperature);

  // TODO garbage?
  void printStorage(word address, byte sizeOf);

  void sendStorageDump();

};

extern WebServerClass GB_WebServer;

#endif



