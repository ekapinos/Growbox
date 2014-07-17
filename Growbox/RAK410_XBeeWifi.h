#ifndef RAK410_XBeeWifi_h
#define RAK410_XBeeWifi_h

#include "Global.h"
#include "PrintUtils.h"

class RAK410_XBeeWifiClass{

private:

  static const unsigned int WIFI_MAX_SEND_FRAME_SIZE = 1400; // 1400 max from spec
  static const unsigned int WIFI_RESPONSE_DEFAULT_DELAY = 1000; // default delay after "at+" commands 1000ms

  boolean c_useSerialWifi;

  boolean c_restartWifi;

  unsigned int c_autoSizeFrameSize;

  boolean isWifiPrintCommandStarted;

public:

  enum RequestType {
    RAK410_XBEEWIFI_REQUEST_TYPE_NONE,
    RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_CONNECTED,
    RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_DISCONNECTED,
    RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET,
    RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST

  };

  RAK410_XBeeWifiClass();

  boolean isPresent(); // check if the device is present


    /////////////////////////////////////////////////////////////////////
  //                             STARTUP                             //
  /////////////////////////////////////////////////////////////////////

  void updateWiFiStatus();

  boolean startupWebServer();


  /////////////////////////////////////////////////////////////////////
  //                              WEB SERVER                         //
  /////////////////////////////////////////////////////////////////////

  RequestType handleSerialEvent(byte &wifiPortDescriptor, String &input, String &postParams);

  void    sendFixedSizeData(const byte portDescriptor, const __FlashStringHelper* data);

  void    sendFixedSizeFrameStart(const byte portDescriptor, word length);
  void    sendFixedSizeFrameData(const __FlashStringHelper* data);
  boolean sendFixedSizeFrameStop();

  void    sendAutoSizeFrameStart(const byte &wifiPortDescriptor);
  boolean sendAutoSizeFrameData(const byte &wifiPortDescriptor, const __FlashStringHelper* data); 
  boolean sendAutoSizeFrameData(const byte &wifiPortDescriptor, const String &data);
  boolean sendAutoSizeFrameStop();

  boolean sendCloseConnection(const byte portDescriptor);


private:
  boolean startupWebServerSilent();

  boolean wifiExecuteCommand(const __FlashStringHelper* command = 0, size_t maxResponseDeleay = WIFI_RESPONSE_DEFAULT_DELAY);
  String wifiExecuteRawCommand(const __FlashStringHelper* command = 0, size_t maxResponseDeleay = WIFI_RESPONSE_DEFAULT_DELAY);

  template <class T> void showWifiMessage(T str, boolean newLine = true){
    if (g_useSerialMonitor){
      Serial.print(F("WIFI> "));
      Serial.print(str);
      if (newLine){  
        Serial.println();
      }      
    }
  }

  template <class T> unsigned int wifiExecuteRawCommandPrint(T command){
    unsigned int rez = Serial1.print(command);  

    if (g_useSerialMonitor){
      if (isWifiPrintCommandStarted){  
        Serial.print(command); 
      } 
      else {
        showWifiMessage(command, false);
      }  
    }
    isWifiPrintCommandStarted = true;
    
    return rez;
  }  

};

extern RAK410_XBeeWifiClass RAK410_XBeeWifi;

#endif





























