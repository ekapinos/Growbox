#include "StringUtils.h"

/////////////////////////////////////////////////////////////////////
//                         FALASH STRINGS                          //
/////////////////////////////////////////////////////////////////////

int StringUtils::flashStringLength(const char PROGMEM* pstr){ 
  return strlen_P(pstr);
}

char StringUtils::flashStringCharAt(const char PROGMEM* pstr, int index, boolean checkOverflow){ 
  if (checkOverflow){
    if (index >= flashStringLength(pstr)){
      return 0xFF; 
    }
  }
  return pgm_read_byte(pstr+index);
}

boolean StringUtils::flashStringEquals(const String &str, const char PROGMEM* pstr){ 
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

boolean StringUtils::flashStringEquals(const char* cstr, size_t cstr_length, const char PROGMEM* pstr){ 
  if (cstr_length != flashStringLength(pstr)){
    return false;
  }
  return (strncmp_P(cstr, pstr, cstr_length) == 0); // check this method
}

boolean StringUtils::flashStringStartsWith(const String &str, const char PROGMEM* pstr){ 
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

boolean StringUtils::flashStringStartsWith(const char* cstr, size_t cstr_length, const char PROGMEM* pstr){ 
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

boolean StringUtils::flashStringEndsWith(const String &str, const char PROGMEM* pstr){ 
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

String StringUtils::flashStringLoad(const char PROGMEM* pstr){
  int length = flashStringLength(pstr);

  String str;
  str.reserve(length);
  for (int i = 0; i< length; i++){
    str += flashStringCharAt(pstr, i, false);
  }
  return str; 
}

String StringUtils::flashStringLoad(const __FlashStringHelper* fstr){ 
  return flashStringLoad((const char PROGMEM*) fstr);
}
boolean StringUtils::flashStringStartsWith(const String &str, const __FlashStringHelper* fstr){
  return flashStringStartsWith(str, (const char PROGMEM*) fstr);
}
boolean StringUtils::flashStringStartsWith(const char* cstr, size_t cstr_length, const __FlashStringHelper* fstr){ 
  return flashStringStartsWith(cstr, cstr_length, (const char PROGMEM*) fstr); 
}
boolean StringUtils::flashStringEquals(const String &str, const __FlashStringHelper* fstr){
  return flashStringEquals(str, (const char PROGMEM*) fstr); 
}
int StringUtils::flashStringLength(const __FlashStringHelper* fstr){ 
  return flashStringLength((const char PROGMEM *) fstr);
}
char StringUtils::flashStringCharAt(const __FlashStringHelper* fstr, int index){ 
  return flashStringCharAt((const char PROGMEM*) fstr, index);
}
boolean StringUtils::flashStringEquals(const char* cstr, size_t length, const __FlashStringHelper* fstr){   
  return flashStringEquals(cstr, length, (const char PROGMEM*) fstr);
}




String StringUtils::getFixedDigitsString(const int number, const byte numberOfDigits){
  String out;
  for (int i = 0; i< numberOfDigits; i++){
    out +='0';
  }
  out += number;
  return out.substring(out.length()-numberOfDigits);
}

String StringUtils::getHEX(byte number, boolean addPrefix){
  String out(number, HEX);
  out.toUpperCase();
  if(number < 0x10){
    out = String('0') + out;
  }
  if (addPrefix){
    out = StringUtils::flashStringLoad(F("0x")) + out;
  }
  return out;
}

String StringUtils::floatToString(float number){
  String out;

  int temp = number*100;
  int whole = temp/100;
  int fract = temp%100;

  out += whole;
  out += '.';
  out += getFixedDigitsString(temp,2);
  return out;
}


String StringUtils::getTimeString(time_t time){
  String out;

  tmElements_t tm;
  breakTime(time, tm);

  out += '[';
  out += getFixedDigitsString(tm.Hour, 2);
  out += ':';
  out += getFixedDigitsString(tm.Minute, 2);
  out += ':';
  out += getFixedDigitsString(tm.Second, 2);
  out += ' ';
  out += getFixedDigitsString(tm.Day, 2);
  out +='.';
  out += getFixedDigitsString(tm.Month, 2);
  out += '.';
  out += getFixedDigitsString(tmYearToCalendar(tm.Year), 4); 
  out += ']';
  return out;
} 






