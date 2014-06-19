#ifndef GB_BootRecord_h
#define GB_BootRecord_h

#include "Storage.h"
#include "LogRecord.h"

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

const byte BOOT_RECORD_SIZE = 32;
const word MAGIC_NUMBER = 0xAA55;

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

  boolean initOnStartup(){
    GB_Storage::read(0, this, sizeof(BootRecord));
    boolean isRestart = isCorrect();
    if (isRestart){
      lastStartupTimeStamp = now();
      GB_Storage::write(OFFSETOF(BootRecord, lastStartupTimeStamp), &(lastStartupTimeStamp), sizeof(lastStartupTimeStamp));   
    } 
    else {
      initAtFirstStartUp();
    }
    return isRestart;
  }

  void cleanupLog(){
    nextLogRecordAddress = sizeof(BootRecord);
    GB_Storage::write(OFFSETOF(BootRecord, nextLogRecordAddress), &(nextLogRecordAddress), sizeof(nextLogRecordAddress)); 
    boolPreferencies.isLogOverflow = false;
    GB_Storage::write(OFFSETOF(BootRecord, boolPreferencies), &(boolPreferencies), sizeof(boolPreferencies));
  }

  void increaseNextLogRecordAddress(){
    nextLogRecordAddress += sizeof(LogRecord);  
    if (nextLogRecordAddress+sizeof(LogRecord) >= GB_Storage::CAPACITY){
      nextLogRecordAddress = sizeof(BootRecord);
      if (!boolPreferencies.isLogOverflow){
        boolPreferencies.isLogOverflow = true;
        GB_Storage::write(OFFSETOF(BootRecord, boolPreferencies), &(boolPreferencies), sizeof(boolPreferencies)); 
      }
    }
    GB_Storage::write(OFFSETOF(BootRecord, nextLogRecordAddress), &(nextLogRecordAddress), sizeof(nextLogRecordAddress)); 
  }

  void setLoggerEnable(boolean flag){
    boolPreferencies.isLoggerEnabled = flag;
    GB_Storage::write(OFFSETOF(BootRecord, boolPreferencies), &(boolPreferencies), sizeof(boolPreferencies)); 

  }

  boolean isCorrect(){
    return (first_magic == MAGIC_NUMBER) && (last_magic == MAGIC_NUMBER);
  }
private:
  void initAtFirstStartUp(){
    first_magic = MAGIC_NUMBER;
    firstStartupTimeStamp = now();
    lastStartupTimeStamp = firstStartupTimeStamp;
    nextLogRecordAddress = sizeof(BootRecord);
    boolPreferencies.isLogOverflow = false;
    boolPreferencies.isLoggerEnabled = true;
    for(byte i=0; i<sizeof(reserved); i++){
      reserved[i] = 0;
    }
    last_magic = MAGIC_NUMBER;
    GB_Storage::write(0, this, sizeof(BootRecord));
  }
};

extern BootRecord BOOT_RECORD;

#endif