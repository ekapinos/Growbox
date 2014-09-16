#ifndef ArduinoPatch_h
#define ArduinoPatch_h

#include <Arduino.h> 

/////////////////////////////////////////////////////////////////////
//                          SERIAL READ                            //
/////////////////////////////////////////////////////////////////////

const unsigned long STREAM_TIMEOUT = 1000; // Like in Stram.h

// WARNING! This is adapted copy of Stream.h, Serial.h, and HardwareSerial.h
// functionality
boolean Serial_timedRead(char* c);

size_t Serial_skipAll();

size_t Serial_skipBytes(size_t length);
size_t Serial_skipBytesUntil(size_t length, const __FlashStringHelper* fstr);
size_t Serial_readBytes(char *buffer, size_t length);

size_t Serial_readStringUntil(String& str, size_t length, const __FlashStringHelper* fstr);
size_t Serial_readString(String& str, size_t length);
size_t Serial_readString(String& str);

#endif

