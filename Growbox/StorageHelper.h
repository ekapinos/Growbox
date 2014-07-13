#ifndef GB_StorageHelper_h
#define GB_StorageHelper_h

#include "StorageModel.h"
#include "AT24C32_EEPROM.h"

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

class StorageHelperClass{

public:

   static const word LOG_CAPACITY = ((AT24C32_EEPROM_Class::CAPACITY - sizeof(BootRecord))/sizeof(LogRecord));

private:
   static const word LOG_RECORD_OVERFLOW_OFFSET = LOG_CAPACITY * sizeof(LogRecord);
   BootRecord bootRecord;

public:
  /////////////////////////////////////////////////////////////////////
  //                            BOOT RECORD                          //
  /////////////////////////////////////////////////////////////////////

   boolean start();

   void setStoreLogRecordsEnabled(boolean flag);
   boolean isStoreLogRecordsEnabled();

   time_t getFirstStartupTimeStamp();
   time_t getLastStartupTimeStamp();

  /////////////////////////////////////////////////////////////////////
  //                            LOG RECORDS                          //
  /////////////////////////////////////////////////////////////////////

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

private :

   boolean isBootRecordCorrect();

   void increaseLogPointer();

};

extern StorageHelperClass GB_StorageHelper;

#endif











