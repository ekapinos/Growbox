#include "StorageHelper.h" 

#include "StringUtils.h"
#include "EEPROM_ARDUINO.h"
#include "EEPROM_AT24C32.h"


#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

const word StorageHelperClass::LOG_CAPACITY_ARDUINO     = (EEPROM.getCapacity() - sizeof(BootRecord)) / sizeof(LogRecord);
const word StorageHelperClass::LOG_CAPACITY_AT24C32     = (EEPROM_AT24C32.getCapacity()) / sizeof(LogRecord);
const word StorageHelperClass::LOG_CAPACITY             = (LOG_CAPACITY_ARDUINO + LOG_CAPACITY_AT24C32);

/////////////////////////////////////////////////////////////////////
//                            BOOT RECORD                          //
/////////////////////////////////////////////////////////////////////

boolean StorageHelperClass::init(){

  BootRecord bootRecord = EEPROM.readBlock<BootRecord>(0);

  if ((bootRecord.first_magic == MAGIC_NUMBER) && (bootRecord.last_magic == MAGIC_NUMBER)){ 
    EEPROM.updateBlock<time_t>(OFFSETOF(BootRecord, lastStartupTimeStamp), now());      
    return true;   
  } 
  else {
    bootRecord.first_magic = MAGIC_NUMBER;
    bootRecord.firstStartupTimeStamp = now();
    bootRecord.lastStartupTimeStamp = bootRecord.firstStartupTimeStamp;
    bootRecord.nextLogRecordIndex = 0;
    bootRecord.boolPreferencies.isLogOverflow = false;
    bootRecord.boolPreferencies.isLoggerEnabled = true;

    bootRecord.boolPreferencies.isWifiStationMode = false; // AP by default
    StringUtils::flashStringLoad(bootRecord.wifiSSID, WIFI_SSID_LENGTH, FS(S_WIFI_DEFAULT_SSID));
    StringUtils::flashStringLoad(bootRecord.wifiPass, WIFI_PASS_LENGTH, FS(S_WIFI_DEFAULT_PASS)); 

    for(byte i=0; i < sizeof(bootRecord.reserved); i++){
      bootRecord.reserved[i] = 0;
    }
    bootRecord.last_magic = MAGIC_NUMBER;

    EEPROM.updateBlock<BootRecord>(0, bootRecord);

    return false; 
  }
}

void update(){
  // TODO implement if extermal EEPROM DISCONNECTED
}

time_t StorageHelperClass::getFirstStartupTimeStamp(){
  return EEPROM.readBlock<time_t>(OFFSETOF(BootRecord, firstStartupTimeStamp)); 
}

time_t StorageHelperClass::getLastStartupTimeStamp(){
  return EEPROM.readBlock<time_t>(OFFSETOF(BootRecord, lastStartupTimeStamp));
}

/////////////////////////////////////////////////////////////////////
//                            LOG RECORDS                          //
/////////////////////////////////////////////////////////////////////

void StorageHelperClass::setStoreLogRecordsEnabled(boolean flag){
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.isLoggerEnabled = flag;
  setBoolPreferencies(boolPreferencies);
}

boolean StorageHelperClass::isStoreLogRecordsEnabled(){
  return getBoolPreferencies().isLoggerEnabled;
}

boolean StorageHelperClass::storeLogRecord(LogRecord &logRecord){ 
  boolean storeLog = g_isGrowboxStarted && isStoreLogRecordsEnabled() && EEPROM.isPresent() && EEPROM_AT24C32.isPresent();
  if (!storeLog){
    return false;
  }
  word nextLogRecordIndex = getNextLogRecordIndex();
  if (nextLogRecordIndex < LOG_CAPACITY_ARDUINO){
    word address = sizeof(BootRecord) + nextLogRecordIndex * sizeof(logRecord);
    EEPROM.updateBlock<LogRecord>(address, logRecord);
  } 
  else {
    word address = (nextLogRecordIndex - LOG_CAPACITY_ARDUINO) * sizeof(logRecord);
    EEPROM_AT24C32.updateBlock<LogRecord>(address, logRecord);
  }
  increaseNextLogRecordIndex();
  return true;
}

boolean StorageHelperClass::isLogOverflow(){
  return getBoolPreferencies().isLogOverflow;
}

word StorageHelperClass::getLogRecordsCount(){
  if (isLogOverflow()){
    return LOG_CAPACITY; 
  } 
  else {
    return getNextLogRecordIndex();
  }
}

boolean StorageHelperClass::getLogRecordByIndex(word index, LogRecord &logRecord){
  if (index >= getLogRecordsCount()){
    return false;
  }

  word planeIndex = 0;
  if (isLogOverflow()){
    planeIndex = getNextLogRecordIndex();
  }
  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  planeIndex += index;

  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  if (planeIndex >= LOG_CAPACITY){
    planeIndex -= LOG_CAPACITY;
  }
  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  if (planeIndex < LOG_CAPACITY_ARDUINO){
    logRecord = EEPROM.readBlock<LogRecord>(sizeof(BootRecord) + planeIndex*sizeof(logRecord));
  } 
  else {
    logRecord = EEPROM_AT24C32.readBlock<LogRecord>((planeIndex-LOG_CAPACITY_ARDUINO)*sizeof(logRecord));
  }
  return true;
}

/////////////////////////////////////////////////////////////////////
//                        GROWBOX COMMANDS                         //
/////////////////////////////////////////////////////////////////////

void StorageHelperClass::resetFirmware(){
  EEPROM.updateBlock<word>(OFFSETOF(BootRecord, first_magic), 0x0000);
}

void StorageHelperClass::resetStoredLog(){  
  EEPROM.updateBlock<word>(OFFSETOF(BootRecord, nextLogRecordIndex), 0x0000);

  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.isLogOverflow = false;
  setBoolPreferencies(boolPreferencies);
}

BootRecord StorageHelperClass::getBootRecord(){
  return EEPROM.readBlock<BootRecord>(0);
}


/////////////////////////////////////////////////////////////////////
//                               WI-FI                             //
/////////////////////////////////////////////////////////////////////

boolean StorageHelperClass::isWifiStationMode(){
  return getBoolPreferencies().isWifiStationMode; 
}
String StorageHelperClass::getWifiSSID(){ 
  byte buffer[WIFI_SSID_LENGTH];
  EEPROM.readBlock<byte>(OFFSETOF(BootRecord, wifiSSID), buffer, WIFI_SSID_LENGTH);

  String str;
  for (word i = 0; i< WIFI_SSID_LENGTH; i++){
    char c = buffer[i];
    if (c == 0){
      break;
    }
    str += c; 
  }
  return str;
}

String StorageHelperClass::getWifiPass(){
  byte buffer[WIFI_PASS_LENGTH];
  EEPROM.readBlock<byte>(OFFSETOF(BootRecord, wifiPass), buffer, WIFI_PASS_LENGTH);

  String str;
  for (word i = 0; i< WIFI_PASS_LENGTH; i++){
    char c = buffer[i];
    if (c == 0){
      break;
    }
    str += c; 
  }
  return str;
}

void  StorageHelperClass::setWifiStationMode(boolean isWifiStationMode){
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.isWifiStationMode = isWifiStationMode;
  setBoolPreferencies(boolPreferencies);
}

void  StorageHelperClass::setWifiSSID(String wifiSSID){
  byte buffer[WIFI_SSID_LENGTH];
  wifiSSID.getBytes(buffer, WIFI_SSID_LENGTH);
  if (wifiSSID.length() == WIFI_SSID_LENGTH){
    buffer[WIFI_SSID_LENGTH -1] = wifiSSID[WIFI_SSID_LENGTH -1]; // fix, we last char can lost with Arduino labrary
  }
  EEPROM.updateBlock<byte>(OFFSETOF(BootRecord, wifiSSID), buffer, WIFI_SSID_LENGTH);
}

void  StorageHelperClass::setWifiPass(String wifiPass){
  byte buffer[WIFI_PASS_LENGTH];
  wifiPass.getBytes(buffer, WIFI_PASS_LENGTH);
  if (wifiPass.length() == WIFI_PASS_LENGTH){
    buffer[WIFI_PASS_LENGTH -1] = wifiPass[WIFI_PASS_LENGTH -1]; // fix, we last char can lost with Arduino labrary
  }
  EEPROM.updateBlock<byte>(OFFSETOF(BootRecord, wifiPass), buffer, WIFI_PASS_LENGTH);
}


word StorageHelperClass::getNextLogRecordIndex(){
  return EEPROM.readBlock<word>(OFFSETOF(BootRecord, nextLogRecordIndex));
}

void StorageHelperClass::increaseNextLogRecordIndex(){
  word nextLogRecordIndex = getNextLogRecordIndex();
  nextLogRecordIndex++;
  if (nextLogRecordIndex >= LOG_CAPACITY){
    nextLogRecordIndex = 0;

    BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
    boolPreferencies.isLogOverflow = true;
    setBoolPreferencies(boolPreferencies);
  }
  EEPROM.updateBlock(OFFSETOF(BootRecord, nextLogRecordIndex), nextLogRecordIndex); 
}


BootRecord::BoolPreferencies StorageHelperClass::getBoolPreferencies(){  
  return EEPROM.readBlock<BootRecord::BoolPreferencies>(OFFSETOF(BootRecord, boolPreferencies));
}

void StorageHelperClass::setBoolPreferencies(BootRecord::BoolPreferencies boolPreferencies){
  EEPROM.updateBlock<BootRecord::BoolPreferencies>(OFFSETOF(BootRecord, boolPreferencies), boolPreferencies);
}

StorageHelperClass GB_StorageHelper;






