#ifndef StringUtils_h
#define StringUtils_h

#include <Arduino.h> 

#include <Time.h> 

#define FS(x) (reinterpret_cast<const __FlashStringHelper*>(x))
//#define PS(x) (reinterpret_cast<const char PROGMEM*>(x))

namespace StringUtils {

  /////////////////////////////////////////////////////////////////////
  //                         FLASH STRING UTILS                      //
  /////////////////////////////////////////////////////////////////////

  size_t flashStringLength(const __FlashStringHelper* fstr);
  char flashStringCharAt(const __FlashStringHelper*, size_t index, boolean checkOverflow = true);

  boolean flashStringEquals(const String &str, const __FlashStringHelper* fstr);
  boolean flashStringEquals(const char* cstr, size_t length, const __FlashStringHelper* fstr);
  boolean flashStringStartsWith(const String &str, const __FlashStringHelper* fstr);
  boolean flashStringStartsWith(const char* cstr, size_t cstr_length, const __FlashStringHelper* fstr);
  boolean flashStringEndsWith(const String &str, const __FlashStringHelper* pstr);

  String flashStringLoad(const __FlashStringHelper* fstr);
  size_t flashStringLoad(char* cstr, size_t length, const __FlashStringHelper* fstr);

  /////////////////////////////////////////////////////////////////////
  //                         STRING CLASS UTILS                      //
  /////////////////////////////////////////////////////////////////////

  String getFixedDigitsString(const int number, const byte numberOfDigits);
  String byteToHexString(byte number, boolean addPrefix = false);
  String floatToString(float number);
  String timeStampToString(time_t time, boolean getDate=true, boolean getTime=true);
  String wordTimeToString(const word time);
  byte hexCharToByte(const char hexChar);
}

#endif





