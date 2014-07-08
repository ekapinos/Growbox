#ifndef GB_Global_h
#define GB_Global_h

#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif

#include <OneWire.h>

#define GB_EXTRA_STRINGS

/////////////////////////////////////////////////////////////////////
//                     HARDWARE CONFIGURATION                      //
/////////////////////////////////////////////////////////////////////

// relay pins
const byte LIGHT_PIN = 3;
const byte FAN_PIN = 4;
const byte FAN_SPEED_PIN = 5;

// 1-Wire pins
const byte ONE_WIRE_PIN = 8;

// status pins
const byte USE_SERIAL_MONOTOR_PIN = 11;
const byte ERROR_PIN = 12;
const byte BREEZE_PIN = LED_BUILTIN; //13

/////////////////////////////////////////////////////////////////////
//                     SOFTWARE CONFIGURATION                      //
/////////////////////////////////////////////////////////////////////

// Light scheduler
const byte UP_HOUR = 1;
const byte DOWN_HOUR = 9;  // 16/8

const float TEMPERATURE_DAY = 26.0;  //23-25, someone 27 
const float TEMPERATURE_NIGHT = 22.0;  // 18-22, someone 22
const float TEMPERATURE_CRITICAL = 35.0; // 35 max, 40 die
const float TEMPERATURE_DELTA = 3.0;   // +/-(3-4) Ok

/////////////////////////////////////////////////////////////////////
//                          PIN CONSTANTCS                         //
/////////////////////////////////////////////////////////////////////

// Relay consts
const byte RELAY_ON = LOW, RELAY_OFF = HIGH;

// Serial consts
const byte SERIAL_ON = HIGH, SERIAL_OFF = LOW;

// Fun speed
const byte FAN_SPEED_MIN = RELAY_OFF;
const byte FAN_SPEED_MAX = RELAY_ON;

/////////////////////////////////////////////////////////////////////
//                             DELAY'S                             //
/////////////////////////////////////////////////////////////////////

// Minimum Growbox reaction time
const int UPDATE_THEMPERATURE_STATISTICS_DELAY = 20; //20 sec 
const int UPDATE_WIFI_STATUS_DELAY = 20; //20 sec 
const int UPDATE_SERIAL_MONITOR_STATUS_DELAY = 1;
const int UPDATE_GROWBOX_STATE_DELAY = 5*60; // 5 min 
const int UPDATE_BREEZE_DELAY = 1;

// error blinks in milleseconds and blink sequences
const word ERROR_SHORT_SIGNAL_MS = 100;  // -> 0
const word ERROR_LONG_SIGNAL_MS = 400;   // -> 1
const word ERROR_DELAY_BETWEEN_SIGNALS_MS = 150;


/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
extern OneWire g_oneWirePin;
extern boolean g_isGrowboxStarted;


/////////////////////////////////////////////////////////////////////
//                         GLOBAL STRINGS                          //
/////////////////////////////////////////////////////////////////////

#define FS(x) (__FlashStringHelper*)(x)

const char S_empty[] PROGMEM  = "";
const char S_CRLF[] PROGMEM  = "\r\n";
const char S_CRLFCRLF[] PROGMEM  = "\r\n\r\n";
const char S_WIFI[] PROGMEM  = "WIFI> ";
const char S_connected[] PROGMEM  = " connected";
const char S_disconnected[] PROGMEM  = " disconnected";
const char S_enabled[] PROGMEM  = " enabled";
const char S_disabled[] PROGMEM  = " disabled";
const char S_Temperature[] PROGMEM  = "Temperature";
const char S_Free_memory[] PROGMEM  = "Free memory: ";
const char S_bytes[] PROGMEM  = " bytes";
const char S_PlusMinus [] PROGMEM  = "+/-";
const char S_Next [] PROGMEM  = " > ";
const char S_0x [] PROGMEM  = "0x";

  /////////////////////////////////////////////////////////////////////
  //                         GLOBAL ENUMS                            //
  /////////////////////////////////////////////////////////////////////

enum HTTP_TAG {
  HTTP_TAG_OPEN, HTTP_TAG_CLOSED, HTTP_TAG_SINGLE
};

enum GB_COMMAND_TYPE {
  GB_COMMAND_NONE, GB_COMMAND_SERIAL_MONITOR, GB_COMMAND_HTTP_CONNECTED, GB_COMMAND_HTTP_DISCONNECTED, GB_COMMAND_HTTP_GET, GB_COMMAND_HTTP_POST

};

  /////////////////////////////////////////////////////////////////////
  //                         FALASH STRINGS                          //
  /////////////////////////////////////////////////////////////////////

static int flashStringLength(const char PROGMEM* pstr){ 
  return strlen_P(pstr);
}

static char flashStringCharAt(const char PROGMEM* pstr, int index, boolean checkOverflow = true){ 
  if (checkOverflow){
    if (index >= flashStringLength(pstr)){
      return 0xFF; 
    }
  }
  return pgm_read_byte(pstr+index);
}

static boolean flashStringEquals(const String &str, const char PROGMEM* pstr){ 
  int length = flashStringLength(pstr);
  if (length != str.length()) {
    return false; 
  }
  for (int i = 0; i < length; i++){
    if (flashStringCharAt(pstr, i, false) != str[i]){
      return false;
    }
  }
  return true;
}

static boolean flashStringEquals(const char* cstr, size_t cstr_length, const char PROGMEM* pstr){ 
  if (cstr_length != flashStringLength(pstr)){
    return false;
  }
  return (strncmp_P(cstr, pstr, cstr_length) == 0); // check this method
}

static boolean flashStringStartsWith(const String &str, const char PROGMEM* pstr){ 
  int length = flashStringLength(pstr);
  if (length > str.length()) {
    return false; 
  }
  for (int i = 0; i < length; i++){
    if (flashStringCharAt(pstr, i, false) != str[i]){
      return false;
    }
  }
  return true; 
}

static boolean flashStringStartsWith(const char* cstr, size_t cstr_length, const char PROGMEM* pstr){ 
  int length = flashStringLength(pstr);
  if (length > cstr_length) {
    return false; 
  }
  for (int i = 0; i < length; i++){
    if (flashStringCharAt(pstr, i, false) != cstr[i]){
      return false;
    }
  }
  return true; 
}

static boolean flashStringEndsWith(const String &str, const char PROGMEM* pstr){ 
  int length = flashStringLength(pstr);
  if (length > str.length()) {
    return false; 
  }
  int strOffset = str.length() - length;
  for (int i = 0; i < length; i++){
    if (flashStringCharAt(pstr, i, false) != str[strOffset+i]){
      return false;
    }
  }
  return true; 
}

static String flashStringLoad(const char PROGMEM* pstr){
  int length = flashStringLength(pstr);

  String str;
  str.reserve(length);
  for (int i = 0; i< length; i++){
    str += flashStringCharAt(pstr, i, false);
  }
  return str; 
}

static String flashStringLoad(const __FlashStringHelper* fstr){ 
  return flashStringLoad((const char PROGMEM*) fstr);
}
static boolean flashStringStartsWith(const String &str, const __FlashStringHelper* fstr){
  return flashStringStartsWith(str, (const char PROGMEM*) fstr);
}
static boolean flashStringStartsWith(const char* cstr, size_t cstr_length, const __FlashStringHelper* fstr){ 
  return flashStringStartsWith(cstr, cstr_length, (const char PROGMEM*) fstr); 
}
static boolean flashStringEquals(const String &str, const __FlashStringHelper* fstr){
  return flashStringEquals(str, (const char PROGMEM*) fstr); 
}
static int flashStringLength(const __FlashStringHelper* fstr){ 
  return flashStringLength((const char PROGMEM *) fstr);
}
static char flashStringCharAt(const __FlashStringHelper* fstr, int index){ 
  return flashStringCharAt((const char PROGMEM*) fstr, index);
}
static boolean flashStringEquals(const char* cstr, size_t length, const __FlashStringHelper* fstr){   
  return flashStringEquals(cstr, length, (const char PROGMEM*) fstr);
}

  /////////////////////////////////////////////////////////////////////
  //                          SERIAL READ                            //
  /////////////////////////////////////////////////////////////////////

  static const unsigned long Stream_timeout = 1000; // Like in Stram.h

  // WARNING! This is adapted copy of Stream.h, Serial.h, and HardwareSerial.h
  // functionality
  static boolean Serial_timedRead(char* c){
    unsigned long _startMillis = millis();
    unsigned long _currentMillis;
    do {
      if (Serial.available()){
        *c = (char) Serial.read();
        return true;   
      }
      _currentMillis = millis();
    } 
    while(((_currentMillis - _startMillis) < Stream_timeout) || (_currentMillis < _startMillis));  // Overflow check 
    //while((_currentMillis - _startMillis) < Stream_timeout); 
    //while(millis() - _startMillis < Stream_timeout); 
    return false;     // false indicates timeout
  }

  static size_t Serial_readBytes(char *buffer, size_t length) {
    size_t count = 0;
    while (count < length) {
      if (!Serial_timedRead(buffer)){
        break;
      }
      buffer++;
      count++;
    }
    return count;
  }

  static size_t Serial_skipBytes(size_t length) {
    char c;
    size_t count = 0;
    while (count < length) {
      if (!Serial_timedRead(&c)){
        break;
      }
      count++;
    }
    return count;
  }

  static size_t Serial_skipBytesUntil(size_t length, const char PROGMEM* pstr){   
    int pstr_length = flashStringLength(pstr);   
    char matcher[pstr_length];

    char c;
    size_t count = 0;
    while (count < length) {
      if (!Serial_timedRead(&c)){
        break;
      }
      count++;

      for (int i = 1; i < pstr_length; i++){
        matcher[i-1] = matcher[i];  
      }
      matcher[pstr_length-1] = c;
      if (count >= pstr_length && flashStringEquals(matcher, pstr_length, pstr)){
        break;
      } 
    }
    return count;
  }  

  static size_t Serial_readStringUntil(String& str, size_t length, const char PROGMEM* pstr){      
    char c;
    size_t count = 0;
    while (count < length) {
      if (!Serial_timedRead(&c)){
        break;
      }
      count++;
      str +=c;
      if (flashStringEndsWith(str, pstr)){
        break;
      } 
    }
    return count;
  } 

  static size_t Serial_readString(String& str, size_t length){
    char buffer[length];
    size_t count = Serial_readBytes(buffer, length);
    str.reserve(str.length() + count);
    for (size_t i = 0; i < count; i++) {
      str += buffer[i];  
    }
    return count;
  }

  static size_t Serial_readString(String& str){

    size_t maxFrameLenght = 100; 
    size_t countInFrame = Serial_readString(str, maxFrameLenght);

    size_t count = countInFrame; 

    while (countInFrame == maxFrameLenght){
      countInFrame = Serial_readString(str, maxFrameLenght); 
      count += countInFrame;
    }
    return count;
  }

#endif


