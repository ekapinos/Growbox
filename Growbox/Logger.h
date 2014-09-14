#ifndef Loggger_h
#define Loggger_h

#include "LoggerModel.h"
#include "StorageModel.h"

class LoggerClass {

public:

  /////////////////////////////////////////////////////////////////////
  //                             APPEND                              //
  /////////////////////////////////////////////////////////////////////

  void logEvent(Event &event);
  void logWateringEvent(byte wsIndex, WateringEvent& wateringEvent, byte value);
  
  void logError(Error &error);
  boolean stopLogError(Error &error);

  void logTemperature(byte temperature);


  /////////////////////////////////////////////////////////////////////
  //                              CHECK                              //
  /////////////////////////////////////////////////////////////////////

  String getLogRecordPrefix(const LogRecord &logRecord);
  const __FlashStringHelper* getLogRecordDescription(LogRecord &logRecord);
  String getLogRecordDescriptionSuffix(const LogRecord &logRecord);

  boolean isEvent(const LogRecord &logRecord);
  boolean isWateringEvent(const LogRecord &logRecord);
  boolean isError(const LogRecord &logRecord);
  boolean isTemperature(const LogRecord &logRecord);

private:

  void printLogRecordToSerialMonotior(const LogRecord &logRecord, const __FlashStringHelper* description, const boolean isStored, const byte temperature = 0xFF);

};

extern LoggerClass GB_Logger;

#endif













