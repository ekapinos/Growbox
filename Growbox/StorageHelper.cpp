#include "StorageHelper.h" 

#include "Global.h"
#include "AT24C32_EEPROM.h"

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

/////////////////////////////////////////////////////////////////////
//                            BOOT RECORD                          //
/////////////////////////////////////////////////////////////////////

boolean StorageHelperClass::init(){

  AT24C32_EEPROM.read(0, &c_bootRecord, sizeof(BootRecord));
  if (isBootRecordCorrect()){
    c_bootRecord.lastStartupTimeStamp = now();      
    AT24C32_EEPROM.write(OFFSETOF(BootRecord, lastStartupTimeStamp), &(c_bootRecord.lastStartupTimeStamp), sizeof(c_bootRecord.lastStartupTimeStamp));      
    return true;   
  } 
  else {
    c_bootRecord.first_magic = MAGIC_NUMBER;
    c_bootRecord.firstStartupTimeStamp = now();
    c_bootRecord.lastStartupTimeStamp = c_bootRecord.firstStartupTimeStamp;
    c_bootRecord.nextLogRecordAddress = sizeof(BootRecord);
    c_bootRecord.boolPreferencies.isLogOverflow = false;
    c_bootRecord.boolPreferencies.isLoggerEnabled = true;
    for(byte i=0; i<sizeof(c_bootRecord.reserved); i++){
      c_bootRecord.reserved[i] = 0;
    }
    c_bootRecord.last_magic = MAGIC_NUMBER;

    AT24C32_EEPROM.write(0, &c_bootRecord, sizeof(BootRecord));

    return false; 
  }
}

void StorageHelperClass::setStoreLogRecordsEnabled(boolean flag){
  c_bootRecord.boolPreferencies.isLoggerEnabled = flag;
  AT24C32_EEPROM.write(OFFSETOF(BootRecord, boolPreferencies), &(c_bootRecord.boolPreferencies), sizeof(c_bootRecord.boolPreferencies)); 
}
boolean StorageHelperClass::isStoreLogRecordsEnabled(){
  return c_bootRecord.boolPreferencies.isLoggerEnabled; 
}

time_t StorageHelperClass::getFirstStartupTimeStamp(){
  return c_bootRecord.firstStartupTimeStamp; 
}
time_t StorageHelperClass::getLastStartupTimeStamp(){
  return c_bootRecord.lastStartupTimeStamp; 
}

/////////////////////////////////////////////////////////////////////
//                            LOG RECORDS                          //
/////////////////////////////////////////////////////////////////////

boolean StorageHelperClass::storeLogRecord(LogRecord &logRecord){ 
  boolean storeLog = g_isGrowboxStarted && isBootRecordCorrect() && c_bootRecord.boolPreferencies.isLoggerEnabled && AT24C32_EEPROM.isPresent(); // TODO check in another places
  if (!storeLog){
    return false;
  }
  AT24C32_EEPROM.write(c_bootRecord.nextLogRecordAddress, &logRecord, sizeof(LogRecord));
  increaseLogPointer();
  return true;
}

boolean StorageHelperClass::isLogOverflow(){
  return c_bootRecord.boolPreferencies.isLogOverflow;
}

word StorageHelperClass::getLogRecordsCount(){
  if (c_bootRecord.boolPreferencies.isLogOverflow){
    return LOG_CAPACITY; 
  } 
  else {
    return (c_bootRecord.nextLogRecordAddress - sizeof(BootRecord))/sizeof(LogRecord);
  }
}
boolean StorageHelperClass::getLogRecordByIndex(word index, LogRecord &logRecord){
  if (index >= getLogRecordsCount()){
    return false;
  }

  word logRecordOffset = 0;
  if (c_bootRecord.boolPreferencies.isLogOverflow){
    logRecordOffset = c_bootRecord.nextLogRecordAddress - sizeof(BootRecord);
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
  c_bootRecord.first_magic = 0;
  AT24C32_EEPROM.write(0, &c_bootRecord, sizeof(BootRecord));
}

void StorageHelperClass::resetStoredLog(){
  c_bootRecord.nextLogRecordAddress = sizeof(BootRecord);
  AT24C32_EEPROM.write(OFFSETOF(BootRecord, nextLogRecordAddress), &(c_bootRecord.nextLogRecordAddress), sizeof(c_bootRecord.nextLogRecordAddress)); 

  c_bootRecord.boolPreferencies.isLogOverflow = false;
  AT24C32_EEPROM.write(OFFSETOF(BootRecord, boolPreferencies), &(c_bootRecord.boolPreferencies), sizeof(c_bootRecord.boolPreferencies));
}

BootRecord StorageHelperClass::getBootRecord(){
  return c_bootRecord; // Creates copy of boot record //TODO check it
}

//private :

boolean StorageHelperClass::isBootRecordCorrect(){ // TODO rename it
  return (c_bootRecord.first_magic == MAGIC_NUMBER) && (c_bootRecord.last_magic == MAGIC_NUMBER);
}

void StorageHelperClass::increaseLogPointer(){
  c_bootRecord.nextLogRecordAddress += sizeof(LogRecord); 
  if (c_bootRecord.nextLogRecordAddress >= (sizeof(BootRecord) + LOG_RECORD_OVERFLOW_OFFSET)){
    c_bootRecord.nextLogRecordAddress = sizeof(BootRecord);
    if (!c_bootRecord.boolPreferencies.isLogOverflow){
      c_bootRecord.boolPreferencies.isLogOverflow = true;
      AT24C32_EEPROM.write(OFFSETOF(BootRecord, boolPreferencies), &(c_bootRecord.boolPreferencies), sizeof(c_bootRecord.boolPreferencies)); 
    }
  }
  AT24C32_EEPROM.write(OFFSETOF(BootRecord, nextLogRecordAddress), &(c_bootRecord.nextLogRecordAddress), sizeof(c_bootRecord.nextLogRecordAddress)); 
}

StorageHelperClass GB_StorageHelper;

