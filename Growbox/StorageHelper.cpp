#include "StorageHelper.h" 

#include "StringUtils.h"
#include "EEPROM_ARDUINO.h"
#include "EEPROM_AT24C32.h"


//#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

const word StorageHelperClass::LOG_CAPACITY_ARDUINO     = (EEPROM.getCapacity() - sizeof(BootRecord)) / sizeof(LogRecord);
const word StorageHelperClass::LOG_CAPACITY_AT24C32     = (EEPROM_AT24C32.getCapacity()) / sizeof(LogRecord);
const word StorageHelperClass::LOG_CAPACITY             = (LOG_CAPACITY_ARDUINO + LOG_CAPACITY_AT24C32);

/////////////////////////////////////////////////////////////////////
//                            BOOT RECORD                          //
/////////////////////////////////////////////////////////////////////

boolean StorageHelperClass::init(){

  EEPROM.readBlock<BootRecord>(0, c_bootRecord);

  if (isBootRecordCorrect()){
    c_bootRecord.lastStartupTimeStamp = now();      
    EEPROM.updateBlock(0, c_bootRecord);      
    return true;   
  } 
  else {
    c_bootRecord.first_magic = MAGIC_NUMBER;
    c_bootRecord.firstStartupTimeStamp = now();
    c_bootRecord.lastStartupTimeStamp = c_bootRecord.firstStartupTimeStamp;
    c_bootRecord.nextLogRecordIndex = 0;
    c_bootRecord.boolPreferencies.isLogOverflow = false;
    c_bootRecord.boolPreferencies.isLoggerEnabled = true;

    c_bootRecord.boolPreferencies.isWifiStationMode = false; // AP by default
    StringUtils::flashStringLoad(FS(S_WIFI_DEFAULT_SSID), c_bootRecord.wifiSSID, WIFI_SSID_LENGTH);
    StringUtils::flashStringLoad(FS(S_WIFI_DEFAULT_PASS), c_bootRecord.wifiPass, WIFI_PASS_LENGTH); 

    for(byte i=0; i < sizeof(c_bootRecord.reserved); i++){
      c_bootRecord.reserved[i] = 0;
    }
    c_bootRecord.last_magic = MAGIC_NUMBER;

    EEPROM.updateBlock(0, c_bootRecord);

    return false; 
  }
}

void update(){

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

void StorageHelperClass::setStoreLogRecordsEnabled(boolean flag){
  c_bootRecord.boolPreferencies.isLoggerEnabled = flag;
  EEPROM.updateBlock(0, c_bootRecord); 
}
boolean StorageHelperClass::isStoreLogRecordsEnabled(){
  return c_bootRecord.boolPreferencies.isLoggerEnabled; 
}

boolean StorageHelperClass::storeLogRecord(LogRecord &logRecord){ 
  boolean storeLog = g_isGrowboxStarted && isBootRecordCorrect() && c_bootRecord.boolPreferencies.isLoggerEnabled && EEPROM.isPresent() && EEPROM_AT24C32.isPresent(); // TODO check in another places
  if (!storeLog){
    return false;
  }
  if (c_bootRecord.nextLogRecordIndex < LOG_CAPACITY_ARDUINO){
    EEPROM.updateBlock<LogRecord>(sizeof(BootRecord)+c_bootRecord.nextLogRecordIndex*sizeof(logRecord), logRecord);
  } 
  else {
    EEPROM_AT24C32.updateBlock<LogRecord>((c_bootRecord.nextLogRecordIndex-LOG_CAPACITY_ARDUINO)*sizeof(logRecord), logRecord);
  }
  increaseLogIndex();
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
    return c_bootRecord.nextLogRecordIndex;
  }
}
boolean StorageHelperClass::getLogRecordByIndex(word index, LogRecord &logRecord){
  if (index >= getLogRecordsCount()){
    return false;
  }

  word planeIndex = 0;
  if (c_bootRecord.boolPreferencies.isLogOverflow){
    planeIndex = c_bootRecord.nextLogRecordIndex;
  }
  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  planeIndex += index;

  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  if (planeIndex >= LOG_CAPACITY){
    planeIndex -= LOG_CAPACITY;
  }
  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  if (planeIndex < LOG_CAPACITY_ARDUINO){
    EEPROM.readBlock<LogRecord>(sizeof(BootRecord) + planeIndex*sizeof(logRecord), logRecord);
  } 
  else {
    EEPROM_AT24C32.readBlock<LogRecord>((planeIndex-LOG_CAPACITY_ARDUINO)*sizeof(logRecord), logRecord);
  }
  return true;
}

/////////////////////////////////////////////////////////////////////
//                        GROWBOX COMMANDS                         //
/////////////////////////////////////////////////////////////////////

void StorageHelperClass::resetFirmware(){
  c_bootRecord.first_magic = 0;
  EEPROM.updateBlock(0, c_bootRecord);
}

void StorageHelperClass::resetStoredLog(){
  c_bootRecord.nextLogRecordIndex = 0;
  c_bootRecord.boolPreferencies.isLogOverflow = false;
  EEPROM.updateBlock(0, c_bootRecord);
}

BootRecord StorageHelperClass::getBootRecord(){
  return c_bootRecord; // Creates copy of boot record //TODO check it
}


/////////////////////////////////////////////////////////////////////
//                               WI-FI                             //
/////////////////////////////////////////////////////////////////////

boolean StorageHelperClass::isWifiStationMode(){
  return c_bootRecord.boolPreferencies.isWifiStationMode; 
}
String StorageHelperClass::getWifiSSID(){
  String str;
  for (word i = 0; i< WIFI_SSID_LENGTH; i++){
    char c = c_bootRecord.wifiSSID[i];
    if (c == 0){
      break;
    }
    str += c; 
  }
  return str;
}

String StorageHelperClass::getWifiPASS(){
    String str;
  for (word i = 0; i< WIFI_PASS_LENGTH; i++){
    char c = c_bootRecord.wifiPass[i];
    if (c == 0){
      break;
    }
    str += c; 
  }
  return str;
}
//private :

boolean StorageHelperClass::isBootRecordCorrect(){ // TODO rename it
  return (c_bootRecord.first_magic == MAGIC_NUMBER) && (c_bootRecord.last_magic == MAGIC_NUMBER);
}

void StorageHelperClass::increaseLogIndex(){
  c_bootRecord.nextLogRecordIndex++;
  if (c_bootRecord.nextLogRecordIndex >= LOG_CAPACITY){
    c_bootRecord.nextLogRecordIndex = 0;
    if (!c_bootRecord.boolPreferencies.isLogOverflow){
      c_bootRecord.boolPreferencies.isLogOverflow = true;
      //EEPROM.updateBlock(0, c_bootRecord); 
    }
  }
  EEPROM.updateBlock(0, c_bootRecord); 
}

StorageHelperClass GB_StorageHelper;


