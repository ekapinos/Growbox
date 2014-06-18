#ifndef GB_Loggger_h
#define GB_Loggger_h

#include "LogRecord.h"
#include "BootRecord.h"
#include "Error.h"
#include "Event.h"
#include "Storage.h"
#include "SerialMonitor.h"

class LoggerClass {
public:
  // Normal event uses uses format [00DDDDDD]
  //   00 - prefix for normal events 
  //   DDDDDD - event identificator
  void logEvent(Event &event){
    LogRecord logRecord(event.index);
    logRawData(logRecord, event.description, true);
  }

  // Error events uses format [01SSDDDD] 
  //   01 - prefix for error events 
  //   SS - length of errir seqence 
  //   DDDD - sequence data
  void logError(Error &error){
    LogRecord logRecord(B01000000|(B00000011 | error.sequenceSize-1)<<4 | (B00001111 & error.sequence));

    logRawData(logRecord, error.description, !error.isStored);
    error.isStored = true;   
    error.notify();
  }
  boolean stopLogError(Error &error){
    if (error.isStored){
      error.isStored = false;
      return true;
    }
    return false;
  }

  // Termometer events uses format [11TTTTTT].
  //   11 - prefix for termometer events
  //   TTTTTT - temperature [0..2^6] = [0..64]
  void logTemperature(byte temperature){
    LogRecord logRecord(B11000000|temperature);
    logRawData(logRecord, F("Temperature"), true, temperature);
  }

  void printLogRecord(LogRecord &logRecord) {

    byte data = (logRecord.data & B00111111);

    if (isEvent(logRecord)){
      Event* foundItemPtr = Event::findByIndex(data);
      if (foundItemPtr == 0){
        logRawData(logRecord, F("Unknown event"), false);
      } 
      else {
        logRawData(logRecord, foundItemPtr->description, false);
      }

    } 
    else if (isTemperature(logRecord)){
      logRawData(logRecord, F("Temperature"), false, data); 

    } 
    else if (isError(logRecord)){    
      byte sequence = (data & B00001111); 
      byte sequenceSize = (data & B00110000)>>4; 
      Error* foundItemPtr = Error::findByIndex(sequence, sequenceSize);
      if (foundItemPtr == 0){
        logRawData(logRecord, F("Unknown error"), false);
      } 
      else {
        logRawData(logRecord, foundItemPtr->description, false);
      }
    } 
    else {
      logRawData(logRecord, F("Unknown"), false);
    }
  }

  static boolean isEvent(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B00000000;
  }
  static boolean isError(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B01000000;
  }
  static boolean isTemperature(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B11000000;
  }

private:

  void logRawData(const LogRecord &logRecord, const __FlashStringHelper* description, boolean storeLog, byte temperature = 0){

    storeLog = storeLog && BOOT_RECORD.isCorrect() && BOOT_RECORD.boolPreferencies.isLoggerEnabled && STORAGE.isPresent();

    if (storeLog) {
      STORAGE.write(BOOT_RECORD.nextLogRecordAddress, &logRecord, sizeof(LogRecord));
      BOOT_RECORD.increaseNextLogRecordAddress();
    }

    if (g_UseSerialMonitor) {
      if (storeLog == false) {
        Serial.print(F("NOT STORED "));
      }
      printTime(logRecord.timeStamp); 
      Serial.print(F(" 0x")); 
      print2digitsHEX(logRecord.data);
      Serial.print(' '); 
      Serial.print(description);
      if (temperature !=0) {
        Serial.print(F(" ["));
        Serial.print((unsigned byte)temperature);
        Serial.print(F("] C"));
      }
      Serial.println();
      printEnd();
    }
  }
};

extern LoggerClass LOGGER;

#endif
