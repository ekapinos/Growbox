#ifndef RAK410_XBeeWifi_h
#define RAK410_XBeeWifi_h

#include "Global.h"
#include "PrintUtils.h"

//#define WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME

class RAK410_XBeeWifiClass{

private:

  static const boolean WIFI_SHOW_AUTO_SIZE_FRAME_DATA = false;

  static const word WIFI_MAX_SEND_FRAME_SIZE = 1400; // 1400 max from spec
  static const word WIFI_RESPONSE_DEFAULT_DELAY = 1000; // default delay after "at+" commands 1000ms

  boolean c_isWifiPresent;
  boolean c_restartWifiOnNextUpdate;
  boolean c_isWifiPrintCommandStarted;
  unsigned int c_autoSizeFrameSize;
  time_t c_lastWifiActivityTimeStamp;
  time_t c_isLastWifiStationMode;
#ifndef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME
  char c_autoSizeFrameBuffer[WIFI_MAX_SEND_FRAME_SIZE];
#endif

public:

  enum RequestType{
    RAK410_XBEEWIFI_REQUEST_TYPE_NONE, RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_CONNECTED, RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_DISCONNECTED, RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET, RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST
  };

  RAK410_XBeeWifiClass();

  boolean isPresent(); // check if the device is present
  void init();
  void update();

private:

  boolean restartWifi(const __FlashStringHelper* description);
  boolean checkStartedWifi();

  /////////////////////////////////////////////////////////////////////
  //                               HTTP                              //
  /////////////////////////////////////////////////////////////////////
public:

  RequestType handleSerialEvent(byte &wifiPortDescriptor, String &input, String &getParams, String &postParams);

  void sendFixedSizeData(const byte portDescriptor, const __FlashStringHelper* data);

  void sendFixedSizeFrameStart(const byte portDescriptor, word length);
  void sendFixedSizeFrameData(const __FlashStringHelper* data);
  void sendFixedSizeFrameData(const String &data);
  boolean sendFixedSizeFrameStop();

  void sendAutoSizeFrameStart(const byte &wifiPortDescriptor);
  boolean sendAutoSizeFrameData(const byte &wifiPortDescriptor, const __FlashStringHelper* data);
  boolean sendAutoSizeFrameData(const byte &wifiPortDescriptor, const String &data);
  boolean sendAutoSizeFrameStop(const byte &wifiPortDescriptor);

  boolean sendCloseConnection(const byte portDescriptor);

private:

  /////////////////////////////////////////////////////////////////////
  //                               TCP                               //
  /////////////////////////////////////////////////////////////////////

  boolean wifiExecuteCommand(const __FlashStringHelper* command = 0, size_t maxResponseDeleay = WIFI_RESPONSE_DEFAULT_DELAY, boolean rebootIfNoResponse=true);
  String wifiExecuteRawCommand(const __FlashStringHelper* command = 0, size_t maxResponseDeleay = WIFI_RESPONSE_DEFAULT_DELAY);

  template <class T> unsigned int wifiExecuteCommandPrint(T command, boolean l_useSerialMonitor = true) {
    unsigned int rez = Serial1.print(command);

    if (g_useSerialMonitor && l_useSerialMonitor) {
      if (c_isWifiPrintCommandStarted) {
        Serial.print(command);
      }
      else {
        showWifiMessage(command, false);
      }
    }
    c_isWifiPrintCommandStarted = true;

    return rez;
  }

  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////

  template <class T> void showWifiMessage(T str, boolean newLine = true) {
    if (g_useSerialMonitor) {
      Serial.print(F("WIFI> "));
      Serial.print(str);
      if (newLine) {
        Serial.println();
      }
    }
  }

};

extern RAK410_XBeeWifiClass RAK410_XBeeWifi;

#endif

