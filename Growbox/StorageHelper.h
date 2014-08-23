#ifndef StorageHelper_h
#define StorageHelper_h

#include "StorageModel.h"

class StorageHelperClass{

private:
  static const word LOG_CAPACITY_ARDUINO;
  static const word LOG_CAPACITY_AT24C32;

  boolean c_isConfigurationLoaded;

  boolean isBoolRecordCorrect(BootRecord& bootRecord);

  BootRecord getBootRecord();
  void setBootRecord(BootRecord bootRecord);
  BootRecord::BoolPreferencies getBoolPreferencies();
  void setBoolPreferencies(BootRecord::BoolPreferencies boolPreferencies);

public:
  StorageHelperClass();

  time_t init_getLastStoredTime();
  boolean init_loadConfiguration(time_t currentTime);

  boolean check_AT24C32_EEPROM();

public:
  time_t getFirstStartupTimeStamp();
  time_t getStartupTimeStamp();
  void adjustFirstStartupTimeStamp(long delta);
  void adjustStartupTimeStamp(long delta);

  void resetFirmware();
  void resetStoredLog();

  void setUseExternal_EEPROM_AT24C32(boolean flag); 
  boolean isUseExternal_EEPROM_AT24C32();

  /////////////////////////////////////////////////////////////////////
  //                              CLOCK                             //
  /////////////////////////////////////////////////////////////////////

  void setUseRTC(boolean flag);
  boolean isUseRTC();

  void setAutoCalculatedClockTimeUsed(boolean flag);
  boolean isAutoCalculatedClockTimeUsed();

  void setAutoAdjustClockTimeDelta(int16_t delta);
  int16_t getAutoAdjustClockTimeDelta();

  void getTurnToDayAndNightTime(word& upTime, word& downTime);
  void setTurnToDayModeTime(const byte upHour, const byte upMinute);
  void setTurnToNightModeTime(const byte downHour, const byte downMinute);

  /////////////////////////////////////////////////////////////////////
  //                            THERMOMETER                          //
  /////////////////////////////////////////////////////////////////////

  void setUseThermometer(boolean flag);
  boolean isUseThermometer();

  void getTemperatureParameters(byte& normalTemperatueDayMin, byte& normalTemperatueDayMax, byte& normalTemperatueNightMin, byte& normalTemperatueNightMax, byte& criticalTemperatue);
  void setNormalTemperatueDayMin(const byte normalTemperatueDayMin);
  void setNormalTemperatueDayMax(const byte normalTemperatueDayMax);
  void setNormalTemperatueNightMin(const byte normalTemperatueNightMin);
  void setNormalTemperatueNightMax(const byte normalTemperatueNightMax);
  void setCriticalTemperatue(const byte criticalTemperatue);

  /////////////////////////////////////////////////////////////////////
  //                               LOGGER                            //
  /////////////////////////////////////////////////////////////////////

  void setStoreLogRecordsEnabled(boolean flag);
  boolean isStoreLogRecordsEnabled();

  boolean storeLogRecord(LogRecord &logRecord);

private:
  void setLogOverflow(boolean flag);

public:
  boolean isLogOverflow();

  word getLogRecordsCapacity();
  word getLogRecordsCount();
  LogRecord getLogRecordByIndex(word index);

private :

  word getNextLogRecordIndex();
  void increaseNextLogRecordIndex();  

public :
  /////////////////////////////////////////////////////////////////////
  //                               WI-FI                             //
  /////////////////////////////////////////////////////////////////////

  boolean isWifiStationMode();
  String getWifiSSID();
  String getWifiPass();

  void setWifiStationMode(boolean);
  void setWifiSSID(String);
  void setWifiPass(String);

  /////////////////////////////////////////////////////////////////////
  //                             WATERING                            //
  /////////////////////////////////////////////////////////////////////

  BootRecord::WateringSystemPreferencies getWateringSystemPreferenciesById(byte id);
  void setWateringSystemPreferenciesById(byte id, BootRecord::WateringSystemPreferencies wateringSystemPreferencies);


};

extern StorageHelperClass GB_StorageHelper;

#endif






