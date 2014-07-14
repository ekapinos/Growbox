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
  boolean c_restartWifiIfNoResponseAutomatically;

  unsigned int c_autoSizeFrameSize;

  String c_wifiSID;
  String c_wifiPass;// 8-63 char

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

  void setConnectionParameters(const String& _s_wifiSID, const String& _s_wifiPass);

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
  void showWifiMessage(const __FlashStringHelper* str, boolean newLine = true);

  boolean startupWebServerSilent();

  boolean wifiExecuteCommand(const __FlashStringHelper* command = 0, size_t maxResponseDeleay = WIFI_RESPONSE_DEFAULT_DELAY);
  String wifiExecuteRawCommand(const __FlashStringHelper* command, size_t maxResponseDeleay = WIFI_RESPONSE_DEFAULT_DELAY);
  
};

extern RAK410_XBeeWifiClass RAK410_XBeeWifi;

#endif
























