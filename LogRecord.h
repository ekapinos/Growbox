#ifndef GB_LogRecord_h
#define GB_LogRecord_h

#include <Time.h>

const byte LOG_RECORD_SIZE = 5;

struct LogRecord {
  time_t timeStamp;
  byte data;  

  LogRecord (byte data): 
  timeStamp(now()), data(data) {
  }

  LogRecord (){
  }
};

#endif


