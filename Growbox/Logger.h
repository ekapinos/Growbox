#ifndef Loggger_h
#define Loggger_h

#include "LoggerModel.h"
#include "StorageModel.h"

class LoggerClass {

public:

  /////////////////////////////////////////////////////////////////////
  //                             APPEND                              //
  /////////////////////////////////////////////////////////////////////

  static void logEvent(Event &event);

  static void logError(Error &error);
  static boolean stopLogError(Error &error);

  static void logTemperature(byte temperature);

  /////////////////////////////////////////////////////////////////////
  //                               READ                              //
  /////////////////////////////////////////////////////////////////////

  static int getLogRecordsCount(); 
  static LogRecord getLogRecordByIndex(int index);

  /////////////////////////////////////////////////////////////////////
  //                              CHECK                              //
  /////////////////////////////////////////////////////////////////////

  static String getLogRecordPrefix(const LogRecord &logRecord);
  static const __FlashStringHelper* getLogRecordDescription(LogRecord &logRecord);
  static String getLogRecordSuffix(const LogRecord &logRecord);

  static boolean isEvent(const LogRecord &logRecord);
  static boolean isError(const LogRecord &logRecord);
  static boolean isTemperature(const LogRecord &logRecord);

private:

  static void printLogRecordToSerialMonotior(const LogRecord &logRecord, const __FlashStringHelper* description, const boolean isStored, const byte temperature = 0xFF);

};

extern LoggerClass GB_Logger;

#endif













