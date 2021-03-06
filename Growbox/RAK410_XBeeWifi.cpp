#include "RAK410_XBeeWifi.h"

#include "Controller.h"
#include "StorageHelper.h"

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

const char S_WIFI_RESPONSE_WELLCOME[] PROGMEM = "Welcome to RAK410\r\n";
const char S_WIFI_RESPONSE_ERROR[] PROGMEM = "ERROR";
const char S_WIFI_RESPONSE_OK[] PROGMEM = "OK";
const char S_WIFI_GET_[] PROGMEM = "GET /";
const char S_WIFI_POST_[] PROGMEM = "POST /";

RAK410_XBeeWifiClass::RAK410_XBeeWifiClass() :
    c_isWifiPresent(false),
    c_restartWifiOnNextUpdate(true),
    c_isWifiPrintCommandStarted(false),
    c_autoSizeFrameSize(0),
    c_lastWifiActivityTimeStamp(0),
    c_isLastWifiStationMode(false) {
}

boolean RAK410_XBeeWifiClass::isPresent() { // check if the device is present
  return c_isWifiPresent;
}

void RAK410_XBeeWifiClass::init() {

  Serial1.begin(115200); // Default RAK 410 speed
  while (!Serial1) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  restartWifi(F("init"));
}

void RAK410_XBeeWifiClass::update() {

  // Restart, if need and not restarted before
  if (c_restartWifiOnNextUpdate) {
    restartWifi(F("software error detected"));
  }
  else if (!isPresent()) {
    // check Wi-Fi hot plug to serial port
    restartWifi(F("check hot connect")); // if restartWifi() returns false, we will try to restart on next update()
  }
  else {
    time_t inactiveTime = now() - c_lastWifiActivityTimeStamp;
    if (inactiveTime > WI_FI_AUTO_REBOOT_ON_INACTIVE_DELAY_SEC) {
      // Auto scheduled reboot
      restartWifi(F("auto, inactive"));

    } else if (inactiveTime > (3*UPDATE_WEB_SERVER_STATUS_DELAY_SEC/4)) { // we skip scheduled check, if Wi-Fi in use now
      // Check connection state
      if (!checkStartedWifi()) {
        restartWifi(F("check status failed")); // if restartWifi() returns false, we will try to restart on next update()
      }
    }
  }
}

// private:

boolean RAK410_XBeeWifiClass::restartWifi(const __FlashStringHelper* description) {

  if (g_useSerialMonitor){
    showWifiMessage(F("Reboot Wi-Fi ("), false);
    Serial.print(description);
    Serial.println(")...");
  }
  c_isWifiPresent = false;

  for (byte i = 0; i <= WI_FI_RECONNECT_ATTEMPTS_BEFORE_USE_DEFAULT_PARAMS; i++) { // Sometimes first command returns ERROR. We use two attempts

    String input = wifiExecuteRawCommand(F("at+reset=0"), 500); // spec boot time 210   // NOresponse checked wrong

    if (!StringUtils::flashStringEquals(input, FS(S_WIFI_RESPONSE_WELLCOME))) {
      if (g_useSerialMonitor && input.length() > 0) {
        showWifiMessage(F("Not correct welcome message: "), false);
        PrintUtils::printWithoutCRLF(input);
        Serial.print(F(" > "));
        PrintUtils::printHEX(input);
        Serial.println();
      }
      continue;
    }

    wifiExecuteCommand(F("at+pwrmode=0"));
    //wifiExecuteCommand(F("at+del_data"));
    //wifiExecuteCommand(F("at+storeenable=0"));

    if (!wifiExecuteCommand(F("at+scan=0"), 5000)) {
      continue;
    }

    boolean useDefaultParameters = (i == WI_FI_RECONNECT_ATTEMPTS_BEFORE_USE_DEFAULT_PARAMS);
    if (useDefaultParameters) {
      showWifiMessage(F("Default parameters will be used"));
    }

    String wifiSSID, wifiPass;
    if (useDefaultParameters) {
      wifiSSID = StringUtils::flashStringLoad(FS(S_WIFI_DEFAULT_SSID));
      wifiPass = StringUtils::flashStringLoad(FS(S_WIFI_DEFAULT_PASS));
      c_isLastWifiStationMode = false;
    }
    else {
      wifiSSID = GB_StorageHelper.getWifiSSID();
      wifiPass = GB_StorageHelper.getWifiPass();
      c_isLastWifiStationMode = GB_StorageHelper.isWifiStationMode();
    }

    if (wifiPass.length() > 0) {
      wifiExecuteCommandPrint(F("at+psk="));
      wifiExecuteCommandPrint(wifiPass);
      if (!wifiExecuteCommand()) {
        continue;
      }
    }

    if (c_isLastWifiStationMode) {
      wifiExecuteCommandPrint(F("at+connect="));
      wifiExecuteCommandPrint(wifiSSID);
      if (!wifiExecuteCommand(NULL, 5000, false)) { // If password wrong, no response from RAK 410
        continue;
      }

      if (!wifiExecuteCommand(F("at+ipdhcp=0"), 5000)) {
        continue;
      }
    }
    else {

      // at+ipstatic=<ip>,<mask>,<gateway>,<dns server1>(0 is valid),<dns server2>(0 is valid)\r\n
      if (!wifiExecuteCommand(F("at+ipstatic=192.168.0.1,255.255.0.0,0.0.0.0,0,0"))) {
        continue;
      }

      if (!wifiExecuteCommand(F("at+ipdhcp=1"), 5000)) {
        continue;
      }

      wifiExecuteCommandPrint(F("at+ap="));
      wifiExecuteCommandPrint(wifiSSID);
      wifiExecuteCommandPrint(F(",1"));
      if (!wifiExecuteCommand(0, 5000)) {
        continue;
      }
    }

    /*if (!wifiExecuteCommand(F("at+httpd_open"))){
     continue;
     }*/
    if (!wifiExecuteCommand(F("at+ltcp=80"))) {
      continue;
    }

    c_isWifiPresent = true;
    c_restartWifiOnNextUpdate = false;
    break;
  }

  if (c_isWifiPresent) {
    showWifiMessage(F("Wi-Fi connected"));
  }
  else {
    showWifiMessage(F("Wi-Fi not connected"));
  }

  return c_isWifiPresent;
}

boolean RAK410_XBeeWifiClass::checkStartedWifi() {

  showWifiMessage(F("Checking Wi-Fi status (stand by)..."));
  if (c_isLastWifiStationMode){
    String input = wifiExecuteRawCommand(F("at+con_status"), 500); // Is Wi-Fi OK?

    c_isWifiPresent = StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_OK));

    if (c_isWifiPresent) {
      showWifiMessage(F("Wi-Fi connection OK"));
    }
    else {
      if (g_useSerialMonitor) {
        showWifiMessage(F("Wi-Fi connection LOST: "), false);
        PrintUtils::printWithoutCRLF(input);
        Serial.print(F(" > "));
        PrintUtils::printHEX(input);
        Serial.println();
      }
    }
  } else {
    showWifiMessage(F("Check skipped in Access Point mode"));
  }
  return c_isWifiPresent;
}
// public:

/////////////////////////////////////////////////////////////////////
//                               HTTP                              //
/////////////////////////////////////////////////////////////////////

RAK410_XBeeWifiClass::RequestType RAK410_XBeeWifiClass::handleSerialEvent(byte &wifiPortDescriptor, String &input, String &getParams, String &postParams) {

  //GB_Controller.updateBreeze(); // TODO make better.. see ArduinoPatch.cpp -> boolean Serial_timedRead(char* c)

  c_lastWifiActivityTimeStamp = now();

  wifiPortDescriptor = 0xFF;
  input = getParams = postParams = String();

  input.reserve(100);
  Serial_readString(input, 13); // "at+recv_data="

  if (!StringUtils::flashStringEquals(input, F("at+recv_data="))) {
    // Read data from serial manager
    Serial_readString(input); // at first we should read, after manipulate  

    if (StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_WELLCOME)) || StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_ERROR))) {
      restartWifi(F("hardware"));
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
    }

    if (g_useSerialMonitor) {
      showWifiMessage(F("Receive unknown data: "), false);
      PrintUtils::printWithoutCRLF(input);
      Serial.print(F(" > "));
      PrintUtils::printHEX(input);
      Serial.println();
    }

    return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
  }
  else {
    // WARNING! We need to do it quick. Standard serial buffer capacity only 64 bytes
    Serial_readString(input, 1); // ends with '\r', cause '\n' will be removed
    byte firstRequestHeaderByte = input[13]; //

    if (firstRequestHeaderByte <= 0x07) {
      // Data Received Successfully
      wifiPortDescriptor = firstRequestHeaderByte;

      Serial_readString(input, 8);  // get full request header

      byte lowByteDataLength = input[20];
      byte highByteDataLength = input[21];
      word dataLength = (((word)highByteDataLength) << 8) + lowByteDataLength;

      // Check HTTP type 
      input = String();
      input.reserve(100);
      dataLength -= Serial_readStringUntil(input, dataLength, FS(S_CRLF));

      boolean isGet = StringUtils::flashStringStartsWith(input, FS(S_WIFI_GET_));
      boolean isPost = StringUtils::flashStringStartsWith(input, FS(S_WIFI_POST_));

      if ((isGet || isPost) && StringUtils::flashStringEndsWith(input, FS(S_CRLF))) {

        int firstIndex;
        if (isGet) {
          firstIndex = StringUtils::flashStringLength(FS(S_WIFI_GET_)) - 1;
        }
        else {
          firstIndex = StringUtils::flashStringLength(FS(S_WIFI_POST_)) - 1;
        }
        int lastIndex = input.indexOf(' ', firstIndex);
        if (lastIndex == -1) {
          lastIndex = input.length() - 2; // \r\n
        }
        input = input.substring(firstIndex, lastIndex);

        int indexOfQuestion = input.indexOf('?');
        if (indexOfQuestion != -1) {
          getParams = input.substring(indexOfQuestion + 1);
          input = input.substring(0, indexOfQuestion);
        }

        if (isGet) {
          // We are not interested in this information
          Serial_skipBytes(dataLength);
          Serial_skipBytes(2); // remove end mark 

          if (g_useSerialMonitor) {
            showWifiMessage(F("Receive from ["), false);
            Serial.print(wifiPortDescriptor);
            Serial.print(F("] GET ["));
            Serial.print(input);
            Serial.print(F("], getParams ["));
            Serial.print(getParams);
            Serial.println(']');
          }

          return RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_GET;
        }
        else {
          // Post
          //word dataLength0 = dataLength;
          dataLength -= Serial_skipBytesUntil(dataLength, FS(S_CRLFCRLF)); // skip HTTP header
          //word dataLength1 = dataLength;
          dataLength -= Serial_readStringUntil(postParams, dataLength, FS(S_CRLF)); // read HTTP data;
          // word dataLength2 = dataLength;           
          Serial_skipBytes(dataLength); // skip remained endings

          if (StringUtils::flashStringEndsWith(postParams, FS(S_CRLF))) {
            postParams = postParams.substring(0, input.length() - 2);
          }

          Serial_skipBytes(2); // remove end mark 

          if (g_useSerialMonitor) {
            showWifiMessage(F("Recive from ["), false);
            Serial.print(wifiPortDescriptor);
            Serial.print(F("] POST ["));
            Serial.print(input);
            Serial.print(F("], getParams ["));
            Serial.print(getParams);
            Serial.print(F("], postParams ["));
            Serial.print(postParams);
            Serial.println(']');
          }

          return RAK410_XBEEWIFI_REQUEST_TYPE_DATA_HTTP_POST;
        }
      }
      else {
        // Unknown HTTP request type
        Serial_skipBytes(dataLength); // remove all data
        Serial_skipBytes(2); // remove end mark 
        if (g_useSerialMonitor) {
          showWifiMessage(F("Receive from ["), false);
          Serial.print(wifiPortDescriptor);
          Serial.print(F("] unknown HTTP ["));
          PrintUtils::printWithoutCRLF(input);
          Serial.print(F("] -> ["));
          PrintUtils::printHEX(input);
          Serial.println(']');
        }

        return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
      }

    }
    else if (firstRequestHeaderByte == 0x80) {
      // TCP client connected
      Serial_readString(input, 1);
      wifiPortDescriptor = input[14];
      Serial_skipBytes(8);

      if (g_useSerialMonitor) {
        showWifiMessage(FS(S_Connected), false);
        Serial.print(' ');
        Serial.println(wifiPortDescriptor);
      }

      return RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_CONNECTED;

    }
    else if (firstRequestHeaderByte == 0x81) {
      // TCP client disconnected
      Serial_readString(input, 1);
      wifiPortDescriptor = input[14];
      Serial_skipBytes(8);

      if (g_useSerialMonitor) {
        showWifiMessage(FS(S_Disconnected), false);
        Serial.print(' ');
        Serial.println(wifiPortDescriptor);
      }

      return RAK410_XBEEWIFI_REQUEST_TYPE_CLIENT_DISCONNECTED;

    }
    else if (firstRequestHeaderByte == 0xFF) {
      // Data received Failed
      Serial_skipBytes(2); // remove end mark and exit quick
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;

    }
    else {
      // Unknown packet and it size
      Serial_skipAll();
      return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
    }
  }
  return RAK410_XBEEWIFI_REQUEST_TYPE_NONE;
}

void RAK410_XBeeWifiClass::sendFixedSizeData(const byte portDescriptor, const __FlashStringHelper* data) { // INT_MAX (own test) or 1400 bytes max (Wi-Fi spec restriction)
  int length = StringUtils::flashStringLength(data);
  if (length == 0) {
    return;
  }
  sendFixedSizeFrameStart(portDescriptor, length);
  wifiExecuteCommandPrint(data);
  sendFixedSizeFrameStop();
}

void RAK410_XBeeWifiClass::sendFixedSizeFrameStart(const byte portDescriptor, word length) { // 1400 bytes max (Wi-Fi module spec restriction)
  wifiExecuteCommandPrint(F("at+send_data="));
  wifiExecuteCommandPrint(portDescriptor);
  wifiExecuteCommandPrint(',');
  wifiExecuteCommandPrint(length);
  wifiExecuteCommandPrint(',');
}

void RAK410_XBeeWifiClass::sendFixedSizeFrameData(const __FlashStringHelper* data) {

  GB_Controller.updateBreeze();

  wifiExecuteCommandPrint(data);
}

void RAK410_XBeeWifiClass::sendFixedSizeFrameData(const String &data) {

  GB_Controller.updateBreeze();

  wifiExecuteCommandPrint(data);
}

boolean RAK410_XBeeWifiClass::sendFixedSizeFrameStop() {
  return wifiExecuteCommand(NULL, WIFI_RESPONSE_DEFAULT_DELAY, false); // maybe client disconnected, do not reboot
}

void RAK410_XBeeWifiClass::sendAutoSizeFrameStart(const byte &wifiPortDescriptor) {

#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME

  sendFixedSizeFrameStart(wifiPortDescriptor, WIFI_MAX_SEND_FRAME_SIZE);
  if (!WIFI_SHOW_AUTO_SIZE_FRAME_DATA) {
    Serial.print(F("[...]"));
  }

#endif

  c_autoSizeFrameSize = 0;
}

boolean RAK410_XBeeWifiClass::sendAutoSizeFrameData(const byte &wifiPortDescriptor, const __FlashStringHelper* data) {
  return RAK410_XBeeWifiClass::sendAutoSizeFrameData(wifiPortDescriptor, StringUtils::flashStringLoad(data));
}

boolean RAK410_XBeeWifiClass::sendAutoSizeFrameData(const byte &wifiPortDescriptor, const String &data) {

  GB_Controller.updateBreeze();

  if (data.length() == 0) {
    return true;
  }

  if (c_autoSizeFrameSize + data.length() < WIFI_MAX_SEND_FRAME_SIZE) {
#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME
    c_autoSizeFrameSize += wifiExecuteCommandPrint(data, WIFI_SHOW_AUTO_SIZE_FRAME_DATA);
#else  
    for (unsigned int i = 0; i < data.length(); i++) {
      c_autoSizeFrameBuffer[c_autoSizeFrameSize++] = data[i];
    }
#endif
    return true;
  }

  size_t index = 0;
  while (c_autoSizeFrameSize < WIFI_MAX_SEND_FRAME_SIZE) {
    char c = data[index++];
#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME
    c_autoSizeFrameSize += wifiExecuteCommandPrint(c, WIFI_SHOW_AUTO_SIZE_FRAME_DATA);
#else  
    c_autoSizeFrameBuffer[c_autoSizeFrameSize++] = c;
#endif
  }
  if (!sendAutoSizeFrameStop(wifiPortDescriptor)) {
    return false;
  }
  sendAutoSizeFrameStart(wifiPortDescriptor);

  while (index < data.length()) {
    char c = data[index++];
#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME
    c_autoSizeFrameSize += wifiExecuteCommandPrint(c, WIFI_SHOW_AUTO_SIZE_FRAME_DATA);
#else  
    c_autoSizeFrameBuffer[c_autoSizeFrameSize++] = c;
#endif  
  }

  return true;
}

boolean RAK410_XBeeWifiClass::sendAutoSizeFrameStop(const byte &wifiPortDescriptor) {

#ifdef WIFI_USE_FIXED_SIZE_SUB_FAMES_IN_AUTO_SIZE_FRAME

  if (c_autoSizeFrameSize > 0) {
    while (c_autoSizeFrameSize < WIFI_MAX_SEND_FRAME_SIZE) {
      c_autoSizeFrameSize += Serial1.write(0x00); // Filler 0x00
    }
  }
  return sendFixedSizeFrameStop();

#else
  if (c_autoSizeFrameSize == 0) {
    return true;
  }
  sendFixedSizeFrameStart(wifiPortDescriptor, c_autoSizeFrameSize);
  if (!WIFI_SHOW_AUTO_SIZE_FRAME_DATA) {
    Serial.print(F("[...]"));
  }
  for (word i = 0; i < c_autoSizeFrameSize; i++) {
    wifiExecuteCommandPrint(c_autoSizeFrameBuffer[i], WIFI_SHOW_AUTO_SIZE_FRAME_DATA);
  }
  c_autoSizeFrameSize = 0;
  return sendFixedSizeFrameStop();
#endif

}

boolean RAK410_XBeeWifiClass::sendCloseConnection(const byte wifiPortDescriptor) {
  wifiExecuteCommandPrint(F("at+cls="));
  wifiExecuteCommandPrint(wifiPortDescriptor);
  return wifiExecuteCommand(NULL, WIFI_RESPONSE_DEFAULT_DELAY, false);
}

//private:

boolean RAK410_XBeeWifiClass::wifiExecuteCommand(const __FlashStringHelper* command, size_t maxResponseDeleay, boolean rebootIfNoResponse) {
  String input = wifiExecuteRawCommand(command, maxResponseDeleay);
  if (StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_OK)) && StringUtils::flashStringEndsWith(input, FS(S_CRLF))) {
    c_lastWifiActivityTimeStamp = now();
    return true;
  }
  else if (input.length() == 0) {
    if (rebootIfNoResponse) {
      c_restartWifiOnNextUpdate = true;
    }

    if (g_useSerialMonitor) {
      showWifiMessage(F("No response"), false);
      if (rebootIfNoResponse) {
        Serial.print(F(". Wi-fi will reboot"));
      }
      else {
        Serial.print(F(". Wi-fi skipped it"));
      }
      Serial.println();
    }

    // Nothing to do
  }
  else if (StringUtils::flashStringStartsWith(input, FS(S_WIFI_RESPONSE_ERROR)) && StringUtils::flashStringEndsWith(input, FS(S_CRLF))) {
    if (g_useSerialMonitor) {
      byte errorCode = input[5];
      showWifiMessage(F("Error "), false);
      Serial.print(StringUtils::byteToHexString(errorCode, true));
      Serial.println();
    }
  }
  else {
    if (g_useSerialMonitor) {
      showWifiMessage(NULL, false);
      PrintUtils::printWithoutCRLF(input);
      Serial.print(F(" > "));
      PrintUtils::printHEX(input);
      Serial.println();
    }
  }

  return false;
}

String RAK410_XBeeWifiClass::wifiExecuteRawCommand(const __FlashStringHelper* command, size_t maxResponseDeleay) {

  Serial_skipAll();

  if (command == NULL) {
    Serial1.println();
  }
  else {
    Serial1.println(command);
  }

  if (g_useSerialMonitor) {
    if (c_isWifiPrintCommandStarted) {
      if (command == NULL) {
        Serial.println();
      }
      else {
        Serial.println(command);
      }
    }
    else {
      if (command == NULL) {
        showWifiMessage(F("Empty command"));
      }
      else {
        showWifiMessage(command);
      }
    }
  }
  c_isWifiPrintCommandStarted = false;

  GB_Controller.updateBreeze();

  String input;
  input.reserve(10);
  unsigned long start = millis();
  while(millis() - start <= maxResponseDeleay) {
    if (Serial1.available()) {
      //input += (char) Serial.read(); 
      //input += Serial.readString(); // WARNING! Problems with command at+ipdhcp=0, it returns bytes with minus sign, Error in Serial library
      Serial_readString(input);

      //showWifiMessage(FS(S_empty), false);
      //Serial.println(input);

      GB_Controller.updateBreeze();
    }
  }

  GB_Controller.updateBreeze();

  return input;
}

RAK410_XBeeWifiClass RAK410_XBeeWifi;

