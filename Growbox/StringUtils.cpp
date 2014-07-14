#include "StringUtils.h"
#include "ArduinoPatch.h"

/////////////////////////////////////////////////////////////////////
//                         FLASH STRING UTILS                      //
/////////////////////////////////////////////////////////////////////

size_t StringUtils::flashStringLength(const __FlashStringHelper* fstr){
  return strlen_P(
  PS(fstr)
  );
}

char StringUtils::flashStringCharAt(const __FlashStringHelper* fstr, size_t index, boolean checkOverflow){ 
  if (checkOverflow){
    if (index >= flashStringLength(fstr)){
      return 0xFF; 
    }
  }
  return pgm_read_byte(PS(fstr)+index);
}

boolean StringUtils::flashStringEquals(const String &str, const __FlashStringHelper* fstr){ 
  size_t length = flashStringLength(fstr);
  if (length != str.length()) {
    return false; 
  }
  for (size_t i = 0; i < length; i++){
    if (flashStringCharAt(fstr, i, false) != str[i]){
      return false;
    }
  }
  return true;
}

boolean StringUtils::flashStringEquals(const char* cstr, size_t cstr_length, const __FlashStringHelper* fstr){ 
  if (cstr_length != flashStringLength(fstr)){
    return false;
  }
  return (strncmp_P(cstr, PS(fstr), cstr_length) == 0); // check this method
}

boolean StringUtils::flashStringStartsWith(const String &str, const __FlashStringHelper* fstr){ 
  size_t length = flashStringLength(fstr);
  if (length > str.length()) {
    return false; 
  }
  for (size_t i = 0; i < length; i++){
    if (flashStringCharAt(fstr, i, false) != str[i]){
      return false;
    }
  }
  return true; 
}

boolean StringUtils::flashStringStartsWith(const char* cstr, size_t cstr_length, const __FlashStringHelper* fstr){ 
  size_t length = flashStringLength(fstr);
  if (length > cstr_length) {
    return false; 
  }
  for (size_t i = 0; i < length; i++){
    if (flashStringCharAt(fstr, i, false) != cstr[i]){
      return false;
    }
  }
  return true; 
}

boolean StringUtils::flashStringEndsWith(const String &str, const __FlashStringHelper* fstr){ 
  size_t length = flashStringLength(fstr);
  if (length > str.length()) {
    return false; 
  }
  int strOffset = str.length() - length;
  for (size_t i = 0; i < length; i++){
    if (flashStringCharAt(fstr, i, false) != str[strOffset+i]){
      return false;
    }
  }
  return true; 
}

String StringUtils::flashStringLoad(const __FlashStringHelper* fstr){
  size_t length = flashStringLength(fstr);

  String str;
  str.reserve(length);
  for (size_t i = 0; i< length; i++){
    str += flashStringCharAt(fstr, i, false);
  }
  return str; 
}

/////////////////////////////////////////////////////////////////////
//                         STRING CLASS UTILS                      //
/////////////////////////////////////////////////////////////////////


String StringUtils::getFixedDigitsString(const int number, const byte numberOfDigits){
  String out;
  for (int i = 0; i< numberOfDigits; i++){
    out +='0';
  }
  out += number;
  return out.substring(out.length()-numberOfDigits);
}

String StringUtils::byteToHexString(byte number, boolean addPrefix){
  String out(number, HEX);
  out.toUpperCase();
  if(number < 0x10){
    out = String('0') + out;
  }
  if (addPrefix){
    out = flashStringLoad(F("0x")) + out;
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
  out += getFixedDigitsString(fract, 2);
  return out;
}


String StringUtils::timeToString(time_t time){
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








