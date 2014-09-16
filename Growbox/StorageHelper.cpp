#include "StorageHelper.h" 

#include "StringUtils.h"
#include "Logger.h"
#include "Watering.h"
#include "Thermometer.h"
#include "EEPROM_ARDUINO.h"
#include "EEPROM_AT24C32.h"
#include "Controller.h"

#define OFFSETOF(type, field)  ((unsigned long) &(((type *) 0)->field))

// private:

const word StorageHelperClass::LOG_CAPACITY_ARDUINO = (EEPROM.getCapacity() - sizeof(BootRecord)) / sizeof(LogRecord);
const word StorageHelperClass::LOG_CAPACITY_AT24C32 = (EEPROM_AT24C32.getCapacity()) / sizeof(LogRecord);

StorageHelperClass::StorageHelperClass() :
    c_isConfigurationLoaded(false) {
}

boolean StorageHelperClass::isBoolRecordCorrect(BootRecord& bootRecord) {
  return (bootRecord.first_magic == MAGIC_NUMBER) && (bootRecord.last_magic == MAGIC_NUMBER);
}

BootRecord StorageHelperClass::getBootRecord() {
  return EEPROM.readBlock<BootRecord>(0);
}
void StorageHelperClass::setBootRecord(BootRecord bootRecord) {
  EEPROM.updateBlock<BootRecord>(0, bootRecord);
}
BootRecord::BoolPreferencies StorageHelperClass::getBoolPreferencies() {
  return EEPROM.readBlock<BootRecord::BoolPreferencies>(OFFSETOF(BootRecord, boolPreferencies));
}

void StorageHelperClass::setBoolPreferencies(BootRecord::BoolPreferencies boolPreferencies) {
  EEPROM.updateBlock<BootRecord::BoolPreferencies>(OFFSETOF(BootRecord, boolPreferencies), boolPreferencies);
}

// public:

time_t StorageHelperClass::init_getLastStoredTime() {
  // init was OK, loadConfiguration not call
  BootRecord bootRecord = getBootRecord();
  if (!isBoolRecordCorrect(bootRecord)) {
    return 0;
  }

  // check last boot
  time_t lastStoredTime = getStartupTimeStamp();

  // check last watering event
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++) {
    BootRecord::WateringSystemPreferencies wsp = getWateringSystemPreferenciesById(wsIndex);
    if (wsp.lastWateringTimeStamp > lastStoredTime) {
      lastStoredTime = wsp.lastWateringTimeStamp;
    }
  }

  // check last log record
  word logRecordIndex = getLogRecordsCount() - 1;
  if (logRecordIndex > 0) {
    LogRecord logRecord = getLogRecordByIndex(logRecordIndex);

    // If No External EEPROM zero value doesn't break this code
    if (logRecord.timeStamp > lastStoredTime) {
      lastStoredTime = logRecord.timeStamp;
    }
  }

  return lastStoredTime;
}

boolean StorageHelperClass::init_loadConfiguration(time_t currentTime) {

  BootRecord bootRecord = getBootRecord();

  boolean itWasRestart;
  if ((bootRecord.first_magic == MAGIC_NUMBER) && (bootRecord.last_magic == MAGIC_NUMBER)) {
    EEPROM.updateBlock<time_t>(OFFSETOF(BootRecord, startupTimeStamp), currentTime);
    itWasRestart = true;
  }
  else {
    bootRecord.first_magic = MAGIC_NUMBER;
    bootRecord.firstStartupTimeStamp = currentTime;
    bootRecord.startupTimeStamp = currentTime;
    bootRecord.nextLogRecordIndex = 0;
    bootRecord.boolPreferencies.isLogOverflow = false;
    bootRecord.boolPreferencies.isLoggerEnabled = true;
    bootRecord.boolPreferencies.isWifiStationMode = false; // Access point used by default

    bootRecord.boolPreferencies.isAutoCalculatedClockTimeUsed = false;
    bootRecord.boolPreferencies.useExternal_EEPROM_AT24C32 = EEPROM_AT24C32.isPresent();
    bootRecord.boolPreferencies.useThermometer = GB_Thermometer.isPresent();
    bootRecord.boolPreferencies.useRTC = GB_Controller.isRTCPresent();
    bootRecord.boolPreferencies.useFan = false;
    bootRecord.boolPreferencies.useLight = false;

    bootRecord.turnToDayModeAt = 9 * 60;
    bootRecord.turnToNightModeAt = 21 * 60;
    bootRecord.normalTemperatueDayMin = 22;
    bootRecord.normalTemperatueDayMax = 26;
    bootRecord.normalTemperatueNightMin = 17;
    bootRecord.normalTemperatueNightMax = 21;
    bootRecord.criticalTemperatue = 35;

    for (byte i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++) {

      BootRecord::WateringSystemPreferencies& wsp = bootRecord.wateringSystemPreferencies[i];

      wsp.boolPreferencies.isWetSensorConnected = false;
      wsp.boolPreferencies.isWaterPumpConnected = false;
      wsp.boolPreferencies.useWetSensorForWatering = false;
      wsp.boolPreferencies.skipNextWatering = false;

      wsp.inAirValue = 240;
      wsp.veryDryValue = 200;
      wsp.dryValue = 180;
      wsp.normalValue = 150;
      wsp.wetValue = 100;
      wsp.veryWetValue = 50;

      wsp.dryWateringDuration = 30;     // 30 sec
      wsp.veryDryWateringDuration = 60; // 60 sec

      wsp.startWateringAt = 9 * 60;  // 9 AM

      wsp.lastWateringTimeStamp = 0; // Unknown
    }

    bootRecord.autoAdjustClockTimeDelta = 0;

    StringUtils::flashStringLoad(bootRecord.wifiSSID, WIFI_SSID_LENGTH, FS(S_WIFI_DEFAULT_SSID));
    StringUtils::flashStringLoad(bootRecord.wifiPass, WIFI_PASS_LENGTH, FS(S_WIFI_DEFAULT_PASS));

    for (byte i = 0; i < sizeof(bootRecord.reserved); i++) {
      bootRecord.reserved[i] = 0;
    }
    bootRecord.last_magic = MAGIC_NUMBER;

    setBootRecord(bootRecord);
    itWasRestart = false;
  }

  c_isConfigurationLoaded = true;
  if (itWasRestart) {
    GB_Logger.logEvent(EVENT_RESTART);
  }
  else {
    GB_Logger.logEvent(EVENT_FIRST_START_UP);
  }
  check_AT24C32_EEPROM();

  return itWasRestart;
}

boolean StorageHelperClass::check_AT24C32_EEPROM() {
  if (isUseExternal_EEPROM_AT24C32() && !EEPROM_AT24C32.isPresent()) {
    GB_Logger.logError(ERROR_AT24C32_EEPROM_DISCONNECTED);
    return false;
  }
  else {
    GB_Logger.stopLogError(ERROR_AT24C32_EEPROM_DISCONNECTED);
    return true;
  }
}

// public:
time_t StorageHelperClass::getFirstStartupTimeStamp() {
  return EEPROM.readBlock<time_t>(OFFSETOF(BootRecord, firstStartupTimeStamp));
}

time_t StorageHelperClass::getStartupTimeStamp() {
  return EEPROM.readBlock<time_t>(OFFSETOF(BootRecord, startupTimeStamp));
}

void StorageHelperClass::adjustFirstStartupTimeStamp(long delta) {
  time_t time = EEPROM.readBlock<time_t>(OFFSETOF(BootRecord, firstStartupTimeStamp));
  time += delta;
  EEPROM.updateBlock<time_t>(OFFSETOF(BootRecord, firstStartupTimeStamp), time);
}

void StorageHelperClass::adjustStartupTimeStamp(long delta) {
  time_t time = EEPROM.readBlock<time_t>(OFFSETOF(BootRecord, startupTimeStamp));
  time += delta;
  EEPROM.updateBlock<time_t>(OFFSETOF(BootRecord, startupTimeStamp), time);
}

void StorageHelperClass::resetFirmware() {
  EEPROM.updateBlock<word>(OFFSETOF(BootRecord, first_magic), 0x0000);
}

void StorageHelperClass::resetStoredLog() {
  EEPROM.updateBlock<word>(OFFSETOF(BootRecord, nextLogRecordIndex), 0x0000);
  setLogOverflow(false);
}

void StorageHelperClass::setUseExternal_EEPROM_AT24C32(boolean useExternal) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  if (boolPreferencies.useExternal_EEPROM_AT24C32 == useExternal) {
    return;
  }
  boolPreferencies.useExternal_EEPROM_AT24C32 = useExternal;
  setBoolPreferencies(boolPreferencies);

  // Prepare log
  if (isLogOverflow()) {
    setLogOverflow(false); // records 0..next => saved, next..arduinoCapacity => lost
  }
  if (!useExternal) {
    // Decrease log space
    // we cam use getLogRecordsCount() cause we already invoke setLogOverflow(false)
    if (getLogRecordsCount() > LOG_CAPACITY_ARDUINO) {
      resetStoredLog(); // last record can't be saved without move
    }
  }
}

boolean StorageHelperClass::isUseExternal_EEPROM_AT24C32() {
  return getBoolPreferencies().useExternal_EEPROM_AT24C32;
}

/////////////////////////////////////////////////////////////////////
//                              CLOCK                             //
/////////////////////////////////////////////////////////////////////

void StorageHelperClass::setUseRTC(boolean flag) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.useRTC = flag;
  setBoolPreferencies(boolPreferencies);
}

boolean StorageHelperClass::isUseRTC() {
  return getBoolPreferencies().useRTC;
}

void StorageHelperClass::setAutoCalculatedClockTimeUsed(boolean flag) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.isAutoCalculatedClockTimeUsed = flag;
  setBoolPreferencies(boolPreferencies);
}

boolean StorageHelperClass::isAutoCalculatedClockTimeUsed() {
  return getBoolPreferencies().isAutoCalculatedClockTimeUsed;
}

void StorageHelperClass::setAutoAdjustClockTimeDelta(int16_t delta) {
  EEPROM.writeBlock<int16_t>(OFFSETOF(BootRecord, autoAdjustClockTimeDelta), delta);
}
int16_t StorageHelperClass::getAutoAdjustClockTimeDelta() {
  return EEPROM.readBlock<int16_t>(OFFSETOF(BootRecord, autoAdjustClockTimeDelta));
}

void StorageHelperClass::getTurnToDayAndNightTime(word& upTime, word& downTime) {
  upTime = EEPROM.readBlock<word>(OFFSETOF(BootRecord, turnToDayModeAt));
  downTime = EEPROM.readBlock<word>(OFFSETOF(BootRecord, turnToNightModeAt));
}
void StorageHelperClass::setTurnToDayModeTime(const byte upHour, const byte upMinute) {
  EEPROM.writeBlock<word>(OFFSETOF(BootRecord, turnToDayModeAt), upHour * 60 + upMinute);
}
void StorageHelperClass::setTurnToNightModeTime(const byte downHour, const byte downMinute) {
  EEPROM.writeBlock<word>(OFFSETOF(BootRecord, turnToNightModeAt), downHour * 60 + downMinute);
}

/////////////////////////////////////////////////////////////////////
//                            THERMOMETER                          //
/////////////////////////////////////////////////////////////////////

void StorageHelperClass::setUseThermometer(boolean flag) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.useThermometer = flag;
  setBoolPreferencies(boolPreferencies);
}
boolean StorageHelperClass::isUseThermometer() {
  return getBoolPreferencies().useThermometer;
}

void StorageHelperClass::getTemperatureParameters(byte& normalTemperatueDayMin, byte& normalTemperatueDayMax, byte& normalTemperatueNightMin, byte& normalTemperatueNightMax, byte& criticalTemperatue) {
  normalTemperatueDayMin = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, normalTemperatueDayMin));
  normalTemperatueDayMax = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, normalTemperatueDayMax));
  normalTemperatueNightMin = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, normalTemperatueNightMin));
  normalTemperatueNightMax = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, normalTemperatueNightMax));
  criticalTemperatue = EEPROM.readBlock<byte>(OFFSETOF(BootRecord, criticalTemperatue));
}

void StorageHelperClass::setNormalTemperatueDayMin(const byte normalTemperatueDayMin) {
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, normalTemperatueDayMin), normalTemperatueDayMin);
}
void StorageHelperClass::setNormalTemperatueDayMax(const byte normalTemperatueDayMax) {
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, normalTemperatueDayMax), normalTemperatueDayMax);
}
void StorageHelperClass::setNormalTemperatueNightMin(const byte normalTemperatueNightMin) {
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, normalTemperatueNightMin), normalTemperatueNightMin);
}
void StorageHelperClass::setNormalTemperatueNightMax(const byte normalTemperatueNightMax) {
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, normalTemperatueNightMax), normalTemperatueNightMax);
}
void StorageHelperClass::setCriticalTemperatue(const byte criticalTemperatue) {
  EEPROM.writeBlock<byte>(OFFSETOF(BootRecord, criticalTemperatue), criticalTemperatue);
}

/////////////////////////////////////////////////////////////////////
//                           OTHER DEVICES                         //
/////////////////////////////////////////////////////////////////////

void StorageHelperClass::setUseFan(boolean flag) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.useFan = flag;
  setBoolPreferencies(boolPreferencies);
}
boolean StorageHelperClass::isUseFan() {
  return getBoolPreferencies().useFan;
}

void StorageHelperClass::setUseLight(boolean flag) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.useLight = flag;
  setBoolPreferencies(boolPreferencies);
}
boolean StorageHelperClass::isUseLight() {
  return getBoolPreferencies().useLight;
}

/////////////////////////////////////////////////////////////////////
//                              LOGGER                             //
/////////////////////////////////////////////////////////////////////

void StorageHelperClass::setStoreLogRecordsEnabled(boolean flag) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  if (boolPreferencies.isLoggerEnabled == flag) {
    return;
  }
  boolean isEnabled = (!boolPreferencies.isLoggerEnabled && flag);
  if (!isEnabled) {
    GB_Logger.logEvent(EVENT_LOGGER_DISABLED);
  }
  boolPreferencies.isLoggerEnabled = flag;
  setBoolPreferencies(boolPreferencies);
  if (isEnabled) {
    GB_Logger.logEvent(EVENT_LOGGER_ENABLED);
  }
}

boolean StorageHelperClass::isStoreLogRecordsEnabled() {
  return getBoolPreferencies().isLoggerEnabled;
}

boolean StorageHelperClass::storeLogRecord(LogRecord &logRecord) {
  boolean storeLog = c_isConfigurationLoaded && isStoreLogRecordsEnabled();
  if (!storeLog) {
    return false;
  }
  word nextLogRecordIndex = getNextLogRecordIndex();
  if (nextLogRecordIndex < LOG_CAPACITY_ARDUINO) {
    word address = sizeof(BootRecord) + nextLogRecordIndex * sizeof(logRecord);
    EEPROM.updateBlock<LogRecord>(address, logRecord);
  }
  else {
    if (!check_AT24C32_EEPROM()) {
      return false;
    }
    word address = (nextLogRecordIndex - LOG_CAPACITY_ARDUINO) * sizeof(logRecord);
    EEPROM_AT24C32.updateBlock<LogRecord>(address, logRecord);
  }
  increaseNextLogRecordIndex();
  return true;
}

// private:
void StorageHelperClass::setLogOverflow(boolean flag) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.isLogOverflow = flag;
  setBoolPreferencies(boolPreferencies);
}

// public:
boolean StorageHelperClass::isLogOverflow() {
  return getBoolPreferencies().isLogOverflow;
}

word StorageHelperClass::getLogRecordsCapacity() {
  word capacity = LOG_CAPACITY_ARDUINO;
  if (isUseExternal_EEPROM_AT24C32()) {
    capacity += LOG_CAPACITY_AT24C32;
  }
  return capacity;
}

word StorageHelperClass::getLogRecordsCount() {
  if (isLogOverflow()) {
    return getLogRecordsCapacity();
  }
  else {
    return getNextLogRecordIndex();
  }
}

LogRecord StorageHelperClass::getLogRecordByIndex(word index) {
  if (index >= getLogRecordsCount()) {
    return LogRecord(); // Empty
  }

  word planeIndex = 0;
  if (isLogOverflow()) {
    planeIndex = getNextLogRecordIndex();
  }
  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  planeIndex += index;

  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  if (planeIndex >= getLogRecordsCapacity()) {
    planeIndex -= getLogRecordsCapacity();
  }
  //Serial.print("logRecordOffset"); Serial.println(logRecordOffset);
  if (planeIndex < LOG_CAPACITY_ARDUINO) {
    return EEPROM.readBlock<LogRecord>(sizeof(BootRecord) + planeIndex * sizeof(LogRecord));
  }
  else {
    if (!check_AT24C32_EEPROM()) {
      return LogRecord(); // Empty
    }
    return EEPROM_AT24C32.readBlock<LogRecord>((planeIndex - LOG_CAPACITY_ARDUINO) * sizeof(LogRecord));
  }
}

// private :

word StorageHelperClass::getNextLogRecordIndex() {
  return EEPROM.readBlock<word>(OFFSETOF(BootRecord, nextLogRecordIndex));
}

void StorageHelperClass::increaseNextLogRecordIndex() {
  word nextLogRecordIndex = getNextLogRecordIndex();
  nextLogRecordIndex++;
  if (nextLogRecordIndex >= getLogRecordsCapacity()) {
    nextLogRecordIndex = 0;

    setLogOverflow(true);
  }
  EEPROM.updateBlock(OFFSETOF(BootRecord, nextLogRecordIndex), nextLogRecordIndex);
}

/////////////////////////////////////////////////////////////////////
//                               WI-FI                             //
/////////////////////////////////////////////////////////////////////

// public :

boolean StorageHelperClass::isWifiStationMode() {
  return getBoolPreferencies().isWifiStationMode;
}
String StorageHelperClass::getWifiSSID() {
  char buffer[WIFI_SSID_LENGTH + 1];
  EEPROM.readBlock<char>(OFFSETOF(BootRecord, wifiSSID), buffer, WIFI_SSID_LENGTH);
  buffer[WIFI_SSID_LENGTH] = 0x00;
  return String(buffer);
}

String StorageHelperClass::getWifiPass() {
  char buffer[WIFI_PASS_LENGTH + 1];
  EEPROM.readBlock<char>(OFFSETOF(BootRecord, wifiPass), buffer, WIFI_PASS_LENGTH);
  buffer[WIFI_PASS_LENGTH] = 0x00;
  return String(buffer);
}

void StorageHelperClass::setWifiStationMode(boolean isWifiStationMode) {
  BootRecord::BoolPreferencies boolPreferencies = getBoolPreferencies();
  boolPreferencies.isWifiStationMode = isWifiStationMode;
  setBoolPreferencies(boolPreferencies);
}

void StorageHelperClass::setWifiSSID(String wifiSSID) {
  byte buffer[WIFI_SSID_LENGTH];
  wifiSSID.getBytes(buffer, WIFI_SSID_LENGTH);
  if (wifiSSID.length() == WIFI_SSID_LENGTH) {
    buffer[WIFI_SSID_LENGTH - 1] = wifiSSID[WIFI_SSID_LENGTH - 1]; // fix, we last char can lost with Arduino labrary
  }
  EEPROM.updateBlock<byte>(OFFSETOF(BootRecord, wifiSSID), buffer, WIFI_SSID_LENGTH);
}

void StorageHelperClass::setWifiPass(String wifiPass) {
  byte buffer[WIFI_PASS_LENGTH];
  wifiPass.getBytes(buffer, WIFI_PASS_LENGTH);
  if (wifiPass.length() == WIFI_PASS_LENGTH) {
    buffer[WIFI_PASS_LENGTH - 1] = wifiPass[WIFI_PASS_LENGTH - 1]; // fix, we last char can lost with Arduino labrary
  }
  EEPROM.updateBlock<byte>(OFFSETOF(BootRecord, wifiPass), buffer, WIFI_PASS_LENGTH);
}

/////////////////////////////////////////////////////////////////////
//                             WATERING                            //
/////////////////////////////////////////////////////////////////////

BootRecord::WateringSystemPreferencies StorageHelperClass::getWateringSystemPreferenciesById(byte id) {
  if (id >= MAX_WATERING_SYSTEMS_COUNT) {
    // TODO add error to log
    return BootRecord::WateringSystemPreferencies();
  }
  return EEPROM.readBlock<BootRecord::WateringSystemPreferencies>(OFFSETOF(BootRecord, wateringSystemPreferencies) + id * sizeof(BootRecord::WateringSystemPreferencies));
}

void StorageHelperClass::setWateringSystemPreferenciesById(byte id, BootRecord::WateringSystemPreferencies wateringSystemPreferencies) {
  if (id >= MAX_WATERING_SYSTEMS_COUNT) {
    return;
  }
  EEPROM.updateBlock<BootRecord::WateringSystemPreferencies>(OFFSETOF(BootRecord, wateringSystemPreferencies) + id * sizeof(BootRecord::WateringSystemPreferencies), wateringSystemPreferencies);
}

StorageHelperClass GB_StorageHelper;

