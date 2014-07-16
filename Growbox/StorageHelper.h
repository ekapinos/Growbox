#ifndef StorageHelper_h
#define StorageHelper_h

#include "StorageModel.h"

class StorageHelperClass{

public:

  static const word LOG_CAPACITY_ARDUINO;
  static const word LOG_CAPACITY_AT24C32;
  static const word LOG_CAPACITY;

private:

  BootRecord c_bootRecord; // TODO - do note load in memory permanent 

public:

  /////////////////////////////////////////////////////////////////////
  //                            BOOT RECORD                          //
  /////////////////////////////////////////////////////////////////////

  boolean init();
  void update();

  time_t getFirstStartupTimeStamp();
  time_t getLastStartupTimeStamp();

  /////////////////////////////////////////////////////////////////////
  //                            LOG RECORDS                          //
  /////////////////////////////////////////////////////////////////////

  void setStoreLogRecordsEnabled(boolean flag);
  boolean isStoreLogRecordsEnabled();

  boolean storeLogRecord(LogRecord &logRecord);

  boolean isLogOverflow();

  word getLogRecordsCount();
  boolean getLogRecordByIndex(word index, LogRecord &logRecord);

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
  String getWifiPASS();


private :

  boolean isBootRecordCorrect();
  void increaseLogIndex();

};

extern StorageHelperClass GB_StorageHelper;

#endif














