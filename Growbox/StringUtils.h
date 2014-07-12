#ifndef GB_StringUtils_h
#define GB_StringUtils_h

#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif


/////////////////////////////////////////////////////////////////////
//                         GLOBAL STRINGS                          //
/////////////////////////////////////////////////////////////////////

#define FS(x) (__FlashStringHelper*)(x)

const char S_empty[] PROGMEM  = "";
const char S___ [] PROGMEM  = "  ";
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
//                         FALASH STRINGS                          //
/////////////////////////////////////////////////////////////////////

namespace StringUtils {
  int flashStringLength(const char PROGMEM* pstr);
  
  char flashStringCharAt(const char PROGMEM* pstr, int index, boolean checkOverflow = true);
  boolean flashStringEquals(const String &str, const char PROGMEM* pstr);
  boolean flashStringEquals(const char* cstr, size_t cstr_length, const char PROGMEM* pstr);
  boolean flashStringStartsWith(const String &str, const char PROGMEM* pstr);
  boolean flashStringStartsWith(const char* cstr, size_t cstr_length, const char PROGMEM* pstr);
  boolean flashStringEndsWith(const String &str, const char PROGMEM* pstr);
  String flashStringLoad(const char PROGMEM* pstr);

  String flashStringLoad(const __FlashStringHelper* fstr);
  boolean flashStringStartsWith(const String &str, const __FlashStringHelper* fstr);
  boolean flashStringStartsWith(const char* cstr, size_t cstr_length, const __FlashStringHelper* fstr);
  boolean flashStringEquals(const String &str, const __FlashStringHelper* fstr);
  int flashStringLength(const __FlashStringHelper* fstr);
  char flashStringCharAt(const __FlashStringHelper* fstr, int index);
  boolean flashStringEquals(const char* cstr, size_t length, const __FlashStringHelper* fstr);
}

#endif


