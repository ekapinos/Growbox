#ifndef GB_StorageModel_h
#define GB_StorageModel_h

#include <Time.h>

const word MAGIC_NUMBER = 0xAA55;

const byte BOOT_RECORD_SIZE = 32;
const byte LOG_RECORD_SIZE = 5;

struct BootRecord { // 32 bytes
  word first_magic;                 //  2
  time_t firstStartupTimeStamp;     //  4
  time_t lastStartupTimeStamp;      //  4
  word nextLogRecordAddress;        //  2
  struct  {
boolean isLogOverflow :
    1;  
boolean isLoggerEnabled :
    2;      
//boolean useWifiAccessPointMode :
//    3;     
  } 
  boolPreferencies;                 //  1 
  byte reserved[17];                //  

  word last_magic;                  //  2  
};

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



