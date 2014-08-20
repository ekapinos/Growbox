#ifndef StorageHelper_h
#define StorageHelper_h

#include "StorageModel.h"

class StorageHelperClass{

  static const word LOG_CAPACITY_ARDUINO;
  static const word LOG_CAPACITY_AT24C32;
  static const word LOG_CAPACITY;

  boolean c_isConfigurationLoaded;

  boolean isStorageHardwarePresent();
  boolean isBoolRecordCorrect(BootRecord& bootRecord);
  boolean isConfigurationLoaded();

  BootRecord getBootRecord();
  void setBootRecord(BootRecord bootRecord);
  BootRecord::BoolPreferencies getBoolPreferencies();
  void setBoolPreferencies(BootRecord::BoolPreferencies boolPreferencies);

public:
  StorageHelperClass();

  boolean init();
  time_t init_getLastStoredTime();
  boolean init_loadConfiguration(time_t currentTime);
  void update();

  time_t getFirstStartupTimeStamp();
  time_t getLastStartupTimeStamp();
  void adjustFirstStartupTimeStamp(long delta);
  void adjustLastStartupTimeStamp(long delta);
  
  void resetFirmware();
  void resetStoredLog();

  void setEEPROM_AT24C32_Connected(boolean flag); 
  boolean isEEPROM_AT24C32_Connected();

  /////////////////////////////////////////////////////////////////////
  //                             GROWBOX                             //
  /////////////////////////////////////////////////////////////////////

  void setClockTimeStampAutoCalculated(boolean flag);
  boolean isClockTimeStampAutoCalculated();

  void getTurnToDayAndNightTime(word& upTime, word& downTime);
  void setTurnToDayModeTime(const byte upHour, const byte upMinute);
  void setTurnToNightModeTime(const byte downHour, const byte downMinute);

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

























