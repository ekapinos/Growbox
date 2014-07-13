#include "StorageHelper.h" 

#include "Global.h"
#include "AT24C32_EEPROM.h"

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

  /////////////////////////////////////////////////////////////////////
  //                            BOOT RECORD                          //
  /////////////////////////////////////////////////////////////////////

   boolean StorageHelperClass::start(){

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

   void StorageHelperClass::setStoreLogRecordsEnabled(boolean flag){
    bootRecord.boolPreferencies.isLoggerEnabled = flag;
    AT24C32_EEPROM.write(OFFSETOF(BootRecord, boolPreferencies), &(bootRecord.boolPreferencies), sizeof(bootRecord.boolPreferencies)); 
  }
   boolean StorageHelperClass::isStoreLogRecordsEnabled(){
    return bootRecord.boolPreferencies.isLoggerEnabled; 
  }

   time_t StorageHelperClass::getFirstStartupTimeStamp(){
    return bootRecord.firstStartupTimeStamp; 
  }
   time_t StorageHelperClass::getLastStartupTimeStamp(){
    return bootRecord.lastStartupTimeStamp; 
  }

  /////////////////////////////////////////////////////////////////////
  //                            LOG RECORDS                          //
  /////////////////////////////////////////////////////////////////////

   boolean StorageHelperClass::storeLogRecord(LogRecord &logRecord){ 
    boolean storeLog = g_isGrowboxStarted && isBootRecordCorrect() && bootRecord.boolPreferencies.isLoggerEnabled && AT24C32_EEPROM.isPresent(); // TODO check in another places
    if (!storeLog){
      return false;
    }
    AT24C32_EEPROM.write(bootRecord.nextLogRecordAddress, &logRecord, sizeof(LogRecord));
    increaseLogPointer();
    return true;
  }

   boolean StorageHelperClass::isLogOverflow(){
    return bootRecord.boolPreferencies.isLogOverflow;
  }

   word StorageHelperClass::getLogRecordsCount(){
    if (bootRecord.boolPreferencies.isLogOverflow){
      return LOG_CAPACITY; 
    } 
    else {
      return (bootRecord.nextLogRecordAddress - sizeof(BootRecord))/sizeof(LogRecord);
    }
  }
   boolean StorageHelperClass::getLogRecordByIndex(word index, LogRecord &logRecord){
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

   void StorageHelperClass::resetFirmware(){
    bootRecord.first_magic = 0;
    AT24C32_EEPROM.write(0, &bootRecord, sizeof(BootRecord));
  }

   void StorageHelperClass::resetStoredLog(){
    bootRecord.nextLogRecordAddress = sizeof(BootRecord);
    AT24C32_EEPROM.write(OFFSETOF(BootRecord, nextLogRecordAddress), &(bootRecord.nextLogRecordAddress), sizeof(bootRecord.nextLogRecordAddress)); 

    bootRecord.boolPreferencies.isLogOverflow = false;
    AT24C32_EEPROM.write(OFFSETOF(BootRecord, boolPreferencies), &(bootRecord.boolPreferencies), sizeof(bootRecord.boolPreferencies));
  }

   BootRecord StorageHelperClass::getBootRecord(){
    return bootRecord; // Creates copy of boot record //TODO check it
  }

//private :

   boolean StorageHelperClass::isBootRecordCorrect(){ // TODO rename it
    return (bootRecord.first_magic == MAGIC_NUMBER) && (bootRecord.last_magic == MAGIC_NUMBER);
  }

   void StorageHelperClass::increaseLogPointer(){
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

StorageHelperClass GB_StorageHelper;
