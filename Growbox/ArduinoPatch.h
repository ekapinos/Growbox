#ifndef ArduinoPatch_h
#define ArduinoPatch_h

#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif

#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data"))) // Workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=34734#c4
#endif

#ifdef PSTR
#undef PSTR
#define PSTR(s) (__extension__({static const char __c[] PROGMEM = (s); &__c[0];})) // Copied from pgmspace.h in avr-libc source
#endif

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

