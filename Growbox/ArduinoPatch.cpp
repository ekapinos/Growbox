#include "ArduinoPatch.h"

#include "StringUtils.h"

boolean Serial_timedRead(char* c){
  unsigned long _startMillis = millis();
  unsigned long _currentMillis;
  do {
    if (Serial1.available()){
      *c = (char) Serial1.read();
      return true;   
    }
    _currentMillis = millis();
  } 
  while(((_currentMillis - _startMillis) < STREAM_TIMEOUT) || (_currentMillis < _startMillis));  // Overflow check 
  //while((_currentMillis - _startMillis) < Stream_timeout); 
  //while(millis() - _startMillis < Stream_timeout); 
  return false;     // false indicates timeout
}

size_t Serial_skipAll(){
  char c;
  size_t count = 0;
  while (true) {
    if (!Serial_timedRead(&c)){
      break;
    }
    count++;
  }
  return count;
}


size_t Serial_skipBytes(size_t length) {
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

size_t Serial_skipBytesUntil(size_t length, const __FlashStringHelper* fstr){   
  size_t fstr_length = StringUtils::flashStringLength(fstr);   
  char matcher[fstr_length];

  char c;
  size_t count = 0;
  while (count < length) {
    if (!Serial_timedRead(&c)){
      break;
    }
    count++;

    for (size_t i = 1; i < fstr_length; i++){
      matcher[i-1] = matcher[i];  
    }
    matcher[fstr_length-1] = c;
    if (count >= fstr_length && StringUtils::flashStringEquals(matcher, fstr_length, fstr)){
      break;
    } 
  }
  return count;
}  

size_t Serial_readBytes(char *buffer, size_t length) {
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

size_t Serial_readStringUntil(String& str, size_t length, const __FlashStringHelper* fstr){      
  char c;
  size_t count = 0;
  while (count < length) {
    if (!Serial_timedRead(&c)){
      break;
    }
    count++;
    str +=c;
    if (StringUtils::flashStringEndsWith(str, fstr)){
      break;
    } 
  }
  return count;
} 

size_t Serial_readString(String& str, size_t length){
  char buffer[length];
  size_t count = Serial_readBytes(buffer, length);
  str.reserve(str.length() + count);
  for (size_t i = 0; i < count; i++) {
    str += buffer[i];  
  }
  return count;
}

size_t Serial_readString(String& str){

  size_t maxFrameLenght = 100; 
  size_t countInFrame = Serial_readString(str, maxFrameLenght);

  size_t count = countInFrame; 

  while (countInFrame == maxFrameLenght){
    countInFrame = Serial_readString(str, maxFrameLenght); 
    count += countInFrame;
  }
  return count;
}
