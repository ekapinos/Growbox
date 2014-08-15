#ifndef StorageHelper_h
#define StorageHelper_h

#include "StorageModel.h"

class StorageHelperClass{

public:

  static const word LOG_CAPACITY_ARDUINO;
  static const word LOG_CAPACITY_AT24C32;
  static const word LOG_CAPACITY;

  /////////////////////////////////////////////////////////////////////
  //                            BOOT RECORD                          //
  /////////////////////////////////////////////////////////////////////

  boolean init();
  boolean loadConfiguration();
  void update();

  time_t getFirstStartupTimeStamp();
  time_t getLastStartupTimeStamp();

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
  //                            LOG RECORDS                          //
  /////////////////////////////////////////////////////////////////////

  void setStoreLogRecordsEnabled(boolean flag);
  boolean isStoreLogRecordsEnabled();
  boolean isStorageHardwarePresent();

  boolean storeLogRecord(LogRecord &logRecord);

  boolean isLogOverflow();

  word getLogRecordsCount();
  LogRecord getLogRecordByIndex(word index);

  /////////////////////////////////////////////////////////////////////
  //                        GROWBOX COMMANDS                         //
  /////////////////////////////////////////////////////////////////////

  void resetFirmware();
  void resetStoredLog();

  BootRecord getBootRecord();

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

private :

  word getNextLogRecordIndex();
  void increaseNextLogRecordIndex();

  BootRecord::BoolPreferencies getBoolPreferencies();
  void setBoolPreferencies(BootRecord::BoolPreferencies boolPreferencies);
};



extern StorageHelperClass GB_StorageHelper;

#endif





















