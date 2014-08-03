#include "StorageHelper.h" 

#include "StringUtils.h"
#include "Logger.h"
#include "Watering.h"
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

    bootRecord.turnToDayModeAt = 9*60;
    bootRecord.turnToNightModeAt = 21*60;
    bootRecord.normalTemperatueDayMin = 22;
    bootRecord.normalTemperatueDayMax = 26;
    bootRecord.normalTemperatueNightMin = 17;
    bootRecord.normalTemperatueNightMax = 21;
    bootRecord.criticalTemperatue = 35;

    for (byte i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++){
      
      BootRecord::WateringSystemPreferencies& wsp = bootRecord.wateringSystemPreferencies[i];
      
      wsp.boolPreferencies.isWetSensorConnected = false;
      wsp.boolPreferencies.isWaterPumpConnected = false;
      
      wsp.notConnectedValue = GB_Watering.analogToByte(1010);
      wsp.veryDryValue      = GB_Watering.analogToByte(900);
      wsp.dryValue          = GB_Watering.analogToByte(800);
      wsp.normalValue       = GB_Watering.analogToByte(600);
      wsp.wetValue          = GB_Watering.analogToByte(500);
      wsp.veryWetValue      = GB_Watering.analogToByte(400);
      wsp.shortCircitValue  = GB_Watering.analogToByte(200);
      
      wsp.dryWateringDuration = 30;     // 30 sec
      wsp.veryDryWateringDuration = 60; // 60 sec
      
      wsp.wateringIfNoSensorAt = 9*60;  // 9 AM   
    }

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

void StorageHelperClass::getTurnToDayAndNightTime(word& upTime, word& downTime){
  upTime   = EEPROM.readBlock<word>(OFFSETOF(BootRecord, turnToDayModeAt));
  downTime = EEPROM.readBlock<word>(OFFSETOF(BootRecord, turnToNightModeAt));
}
void StorageHelperClass::setTurnToDayModeTime(const byte upHour, const byte upMinute){
  EEPROM.writeBlock<word>(OFFSETOF(BootRecord, turnToDayModeAt), upHour*60+upMinute);
}
void StorageHelperClass::setTurnToNightModeTime(const byte downHour, const byte downMinute){
  EEPROM.writeBlock<word>(OFFSETOF(BootRecord, turnToNightModeAt), downHour*60+downMinute);
}

void StorageHelperClass::getTemperatureParameters(byte& normalTemperatueDayMin, byte& normalTemperatueDayMax, byte& normalTemperatueNightMin, byte& normalTemperatueNightMax, byte& criticalTemperatue){
  normalTemperatueDayMin   = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, normalTemperatueDayMin));
  normalTemperatueDayMax   = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, normalTemperatueDayMax));
  normalTemperatueNightMin = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, normalTemperatueNightMin));
  normalTemperatueNightMax = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, normalTemperatueNightMax));
  criticalTemperatue       = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, criticalTemperatue));
}

void StorageHelperClass::setNormalTemperatueDayMin(const byte normalTemperatueDayMin){
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, normalTemperatueDayMin), normalTemperatueDayMin);
}
void StorageHelperClass::setNormalTemperatueDayMax(const byte normalTemperatueDayMax){
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, normalTemperatueDayMax), normalTemperatueDayMax);
}
void StorageHelperClass::setNormalTemperatueNightMin(const byte normalTemperatueNightMin){
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, normalTemperatueNightMin), normalTemperatueNightMin);
}
void StorageHelperClass::setNormalTemperatueNightMax(const byte normalTemperatueNightMax){
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, normalTemperatueNightMax), normalTemperatueNightMax);
}
void StorageHelperClass::setCriticalTemperatue(const byte criticalTemperatue){
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, criticalTemperatue), criticalTemperatue);
}


/////////////////////////////////////////////////////////////////////
//                            LOG RECORDS                          //
/////////////////////////////////////////////////////////////////////

void StorageHelperClass::setStoreLogRecordsEnabled(boolean flag){
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  if (boolPreferencies.isLoggerEnabled == flag){
    return;
  }
  boolean isEnabled = (!boolPreferencies.isLoggerEnabled && flag);
  if (!isEnabled){
    GB_Logger.logEvent(EVENT_LOGGER_DISABLED);
  } 
  boolPreferencies.isLoggerEnabled = flag;
  setBoolPreferencies(boolPreferencies);
  if (isEnabled) {
    GB_Logger.logEvent(EVENT_LOGGER_ENABLED);
  }
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

LogRecord StorageHelperClass::getLogRecordByIndex(word index){
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
    return EEPROM.readBlock<LogRecord>(sizeof(BootRecord) + planeIndex*sizeof(LogRecord));
  } 
  else {
    return EEPROM_AT24C32.readBlock<LogRecord>((planeIndex-LOG_CAPACITY_ARDUINO)*sizeof(LogRecord));
  }
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
  char buffer[WIFI_SSID_LENGTH+1];
  EEPROM.readBlock<char>(OFFSETOF(BootRecord, wifiSSID), buffer, WIFI_SSID_LENGTH);
  buffer[WIFI_SSID_LENGTH] = 0x00;
  return String(buffer);
}

String StorageHelperClass::getWifiPass(){
  char buffer[WIFI_PASS_LENGTH+1];
  EEPROM.readBlock<char>(OFFSETOF(BootRecord, wifiPass), buffer, WIFI_PASS_LENGTH);
  buffer[WIFI_PASS_LENGTH] = 0x00;
  return String(buffer);
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


/////////////////////////////////////////////////////////////////////
//                             WATERING                            //
/////////////////////////////////////////////////////////////////////

BootRecord::WateringSystemPreferencies StorageHelperClass::getWateringSystemPreferenciesById(byte id){
  if (id >= MAX_WATERING_SYSTEMS_COUNT){
    // TODO add error to log
    return BootRecord::WateringSystemPreferencies();
  }
  return EEPROM.readBlock<BootRecord::WateringSystemPreferencies>(OFFSETOF(BootRecord, wateringSystemPreferencies)+id * sizeof(BootRecord::WateringSystemPreferencies));
}

void StorageHelperClass::setWateringSystemPreferenciesById(byte id, BootRecord::WateringSystemPreferencies wateringSystemPreferencies){
  if (id >= MAX_WATERING_SYSTEMS_COUNT){
    return;
  }
  EEPROM.updateBlock<BootRecord::WateringSystemPreferencies>(OFFSETOF(BootRecord, wateringSystemPreferencies)+id * sizeof(BootRecord::WateringSystemPreferencies), wateringSystemPreferencies);
}
    
StorageHelperClass GB_StorageHelper;










