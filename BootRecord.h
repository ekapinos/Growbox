#ifndef GB_BootRecord_h
#define GB_BootRecord_h

#include <Time.h>

#include "Storage.h"
#include "LogRecord.h"

const byte BOOT_RECORD_SIZE = 32;

const word MAGIC_NUMBER = 0xAA55;

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))


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
  } 
  boolPreferencies;                 //  1 
  byte reserved[17];                //  

  word last_magic;                  //  2
  
};

#endif



