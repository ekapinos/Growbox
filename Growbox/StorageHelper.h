#ifndef GB_StorageHelper_h
#define GB_StorageHelper_h

#include "Global.h"
#include "AT24C32_EEPROM.h"
#include "StorageModel.h"

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

class GB_StorageHelper{

public:

  static const word LOG_CAPACITY = ((AT24C32_EEPROM_Class::CAPACITY - sizeof(BootRecord))/sizeof(LogRecord));

private:
  static const word LOG_RECORD_OVERFLOW_OFFSET = LOG_CAPACITY * sizeof(LogRecord);
  static BootRecord bootRecord;

public:
  /////////////////////////////////////////////////////////////////////
  //                            BOOT RECORD                          //
  /////////////////////////////////////////////////////////////////////

  static boolean start(){

    AT24C32_EEPROM.read(0, &bootRecord, sizeof(BootRecord));
    if (isBootRecordCorrect()){
      bootRecord.lastStartupTimeStamp = now();      
      AT24C32_EEPROM.write(OFFSETOF(BootRecord, lastStartupTimeStamp), &(bootRecord.lastStartupTimeStamp), sizeof(bootRecord.lastStartupTimeStamp));      
      return true;   
    } 
    else {
      bootRecord.first_magic = MAGIC_NUMBER;
      bootRecord.firstStartupTimeStamp = now();
      bootRecord.lastStartupTimeStamp = bootRecord.firstStartupTimeStamp;
      bootRecord.nextLogRecordAddress = sizeof(BootRecord);
      bootRecord.boolPreferencies.isLogOverflow = false;
      bootRecord.boolPreferencies.isLoggerEnabled = true;
      for(byte i=0; i<sizeof(bootRecord.reserved); i++){
        bootRecord.reserved[i] = 0;
      }
      bootRecord.last_magic = MAGIC_NUMBER;

      AT24C32_EEPROM.write(0, &bootRecord, sizeof(BootRecord));

      return false; 
    }
  }

  static void setStoreLogRecordsEnabled(boolean flag){
    bootRecord.boolPreferencies.isLoggerEnabled = flag;
    AT24C32_EEPROM.write(OFFSETOF(BootRecord, boolPreferencies), &(bootRecord.boolPreferencies), sizeof(bootRecord.boolPreferencies)); 
  }
  static boolean isStoreLogRecordsEnabled(){
    return bootRecord.boolPreferencies.isLoggerEnabled; 
  }

  static time_t getFirstStartupTimeStamp(){
    return bootRecord.firstStartupTimeStamp; 
  }
  static time_t getLastStartupTimeStamp(){
    return bootRecord.lastStartupTimeStamp; 
  }

  /////////////////////////////////////////////////////////////////////
  //                            LOG RECORDS                          //
  /////////////////////////////////////////////////////////////////////

  static boolean storeLogRecord(LogRecord &logRecord){ 
    boolean storeLog = g_isGrowboxStarted && isBootRecordCorrect() && bootRecord.boolPreferencies.isLoggerEnabled && AT24C32_EEPROM.isPresent(); // TODO check in another places
    if (!storeLog){
      return false;
    }
    AT24C32_EEPROM.write(bootRecord.nextLogRecordAddress, &logRecord, sizeof(LogRecord));
    increaseLogPointer();
    return true;
  }

  static boolean isLogOverflow(){
    return bootRecord.boolPreferencies.isLogOverflow;
  }

  static word getLogRecordsCount(){
    if (bootRecord.boolPreferencies.isLogOverflow){
      return LOG_CAPACITY; 
    } 
    else {
      return (bootRecord.nextLogRecordAddress - sizeof(BootRecord))/sizeof(LogRecord);
    }
  }
  static boolean getLogRecordByIndex(word index, LogRecord &logRecord){
    if (index >= getLogRecordsCount()){
      return false;
    }

    word logRecordOffset = 0;
    if (bootRecord.boolPreferencies.isLogOverflow){
      logRecordOffset = bootRecord.nextLogRecordAddress - sizeof(BootRecord);
    }
    //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
    logRecordOffset += index * sizeof(LogRecord);

    //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
    if (logRecordOffset >= LOG_RECORD_OVERFLOW_OFFSET){
      logRecordOffset -= LOG_RECORD_OVERFLOW_OFFSET;
    }
    //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
    word address = sizeof(BootRecord) + logRecordOffset; 
    AT24C32_EEPROM.read(address, &logRecord, sizeof(LogRecord));  
    return true;
  }

  /////////////////////////////////////////////////////////////////////
  //                        GROWBOX COMMANDS                         //
  /////////////////////////////////////////////////////////////////////

  static void resetFirmware(){
    bootRecord.first_magic = 0;
    AT24C32_EEPROM.write(0, &bootRecord, sizeof(BootRecord));
  }

  static void resetStoredLog(){
    bootRecord.nextLogRecordAddress = sizeof(BootRecord);
    AT24C32_EEPROM.write(OFFSETOF(BootRecord, nextLogRecordAddress), &(bootRecord.nextLogRecordAddress), sizeof(bootRecord.nextLogRecordAddress)); 

    bootRecord.boolPreferencies.isLogOverflow = false;
    AT24C32_EEPROM.write(OFFSETOF(BootRecord, boolPreferencies), &(bootRecord.boolPreferencies), sizeof(bootRecord.boolPreferencies));
  }

  static BootRecord getBootRecord(){
    return bootRecord; // Creates copy of boot record //TODO check it
  }

private :

  static boolean isBootRecordCorrect(){ // TODO rename it
    return (bootRecord.first_magic == MAGIC_NUMBER) && (bootRecord.last_magic == MAGIC_NUMBER);
  }

  static void increaseLogPointer(){
    bootRecord.nextLogRecordAddress += sizeof(LogRecord); 
    if (bootRecord.nextLogRecordAddress >= (sizeof(BootRecord) + LOG_RECORD_OVERFLOW_OFFSET)){
      bootRecord.nextLogRecordAddress = sizeof(BootRecord);
      if (!bootRecord.boolPreferencies.isLogOverflow){
        bootRecord.boolPreferencies.isLogOverflow = true;
        AT24C32_EEPROM.write(OFFSETOF(BootRecord, boolPreferencies), &(bootRecord.boolPreferencies), sizeof(bootRecord.boolPreferencies)); 
      }
    }
    AT24C32_EEPROM.write(OFFSETOF(BootRecord, nextLogRecordAddress), &(bootRecord.nextLogRecordAddress), sizeof(bootRecord.nextLogRecordAddress)); 
  }

};

#endif











