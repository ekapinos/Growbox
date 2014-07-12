#ifndef RAK410_XBeeWifi_h
#define RAK410_XBeeWifi_h

#include "Global.h"
#include "PrintUtils.h"


class RAK410_XBeeWifiClass{
  
private:

  static const int WIFI_MAX_SEND_FRAME_SIZE = 1400; // 1400 max from spec

  static const int WIFI_RESPONSE_DEFAULT_DELAY = 1000; // default delay after "at+" commands 1000ms
  static const unsigned long Stream_timeout = 1000; // Like in Stram.h


  String s_wifiSID;
  String s_wifiPass;// 8-63 char

  boolean s_restartWifi;
  boolean s_restartWifiIfNoResponseAutomatically;

  int s_sendWifiDataFrameSize;

public:

enum RequestType {
  RAK410_XBEEWIFI_REQUEST_TYPE_NONE,
  RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_CONNECTED,
  RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_DISCONNECTED,
  RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET,
  RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST

};

  /*volatile*/  boolean useSerialWifi;

  RAK410_XBeeWifiClass();

  /////////////////////////////////////////////////////////////////////
  //                             STARTUP                             //
  /////////////////////////////////////////////////////////////////////


  void setWifiConfiguration(const String& _s_wifiSID, const String& _s_wifiPass);

  void updateWiFiStatus();

  void checkSerial();

  boolean startWifi();

  void cleanSerialBuffer();

 
   /////////////////////////////////////////////////////////////////////
  //                           WIFI PROTOCOL                         //
  /////////////////////////////////////////////////////////////////////

  RequestType handleSerialEvent(byte &wifiPortDescriptor, String &input, String &postParams);

  void sendWifiFrameStart(const byte portDescriptor, word length);
  void sendWifiFrameData(const __FlashStringHelper* data);
  boolean sendWifiFrameStop();

  void sendWifiData(const byte portDescriptor, const __FlashStringHelper* data);

  void sendWifiDataStart(const byte &wifiPortDescriptor);

  boolean sendWifiAutoFrameData(const byte &wifiPortDescriptor, const __FlashStringHelper* data); 
  boolean sendWifiAutoFrameData(const byte &wifiPortDescriptor, const String &data);

  boolean sendWifiDataStop();

  boolean sendWifiCloseConnection(const byte portDescriptor);


private:
  void showWifiMessage(const __FlashStringHelper* str, boolean newLine = true);

  boolean startWifiSilent();

  boolean wifiExecuteCommand(const __FlashStringHelper* command = 0, int maxResponseDeleay = WIFI_RESPONSE_DEFAULT_DELAY);
  String wifiExecuteRawCommand(const __FlashStringHelper* command, int maxResponseDeleay);
  

private:
  /////////////////////////////////////////////////////////////////////
  //                          SERIAL READ                            //
  /////////////////////////////////////////////////////////////////////

  // WARNING! This is adapted copy of Stream.h, Serial.h, and HardwareSerial.h
  // functionality
  boolean Serial_timedRead(char* c);

  size_t Serial_readBytes(char *buffer, size_t length);

  size_t Serial_skipBytes(size_t length);

  size_t Serial_skipBytesUntil(size_t length, const char PROGMEM* pstr);  

  size_t Serial_readStringUntil(String& str, size_t length, const char PROGMEM* pstr);

  size_t Serial_readString(String& str, size_t length);

  size_t Serial_readString(String& str);


};

extern RAK410_XBeeWifiClass RAK410_XBeeWifi;

#endif























