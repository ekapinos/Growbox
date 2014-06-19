#ifndef GB_Loggger_h
#define GB_Loggger_h

#include "Error.h"
#include "Event.h"
#include "LogRecord.h"
#include "BootRecord.h"
#include "Storage.h"
#include "Print.h"

class GB_Logger {
  
public:

  // Normal event uses uses format [00DDDDDD]
  //   00 - prefix for normal events 
  //   DDDDDD - event identificator
  static void logEvent(Event &event){
    LogRecord logRecord(event.index);
    logRawData(logRecord, event.description, true);
  }

  // Error events uses format [01SSDDDD] 
  //   01 - prefix for error events 
  //   SS - length of errir seqence 
  //   DDDD - sequence data
  static void logError(Error &error){
    LogRecord logRecord(B01000000|(B00000011 | error.sequenceSize-1)<<4 | (B00001111 & error.sequence));

    logRawData(logRecord, error.description, !error.isStored);
    error.isStored = true;   
    error.notify();
  }
  static boolean stopLogError(Error &error){
    if (error.isStored){
      error.isStored = false;
      return true;
    }
    return false;
  }

  // Termometer events uses format [11TTTTTT].
  //   11 - prefix for termometer events
  //   TTTTTT - temperature [0..2^6] = [0..64]
  static void logTemperature(byte temperature){
    LogRecord logRecord(B11000000|temperature);
    logRawData(logRecord, F("Temperature"), true, temperature);
  }

  static void printLogRecord(LogRecord &logRecord) {

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


  static void printFullLog(boolean events, boolean errors, boolean temperature){
    word index = 1;
    if (BOOT_RECORD.boolPreferencies.isLogOverflow) {
      for (word i = BOOT_RECORD.nextLogRecordAddress; i < (GB_Storage::CAPACITY-sizeof(LogRecord)) ; i+=sizeof(LogRecord)){
       // printLogRecord(i, index++, events,  errors,  temperature);
      }
    }
    for (word i = sizeof(BootRecord); i < BOOT_RECORD.nextLogRecordAddress ; i+=sizeof(LogRecord)){
      //printLogRecord(i, index++,  events,  errors,  temperature);
    }
    if (index == 1){
      Serial.println(F("- no records in log"));
    }
  }



private:

  static boolean isEvent(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B00000000;
  }
  static boolean isError(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B01000000;
  }
  static boolean isTemperature(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B11000000;
  }

  static void logRawData(const LogRecord &logRecord, const __FlashStringHelper* description, boolean storeLog, byte temperature = 0){

    storeLog = storeLog && BOOT_RECORD.isCorrect() && BOOT_RECORD.boolPreferencies.isLoggerEnabled && GB_Storage::isPresent();

    if (storeLog) {
      GB_Storage::write(BOOT_RECORD.nextLogRecordAddress, &logRecord, sizeof(LogRecord));
      BOOT_RECORD.increaseLogPointer();
    }

    if (g_UseSerialMonitor) {
      if (storeLog == false) {
        Serial.print(F("NOT STORED "));
      }
      GB_Print::printTime(logRecord.timeStamp); 
      Serial.print(F(" 0x")); 
      GB_Print::print2digitsHEX(logRecord.data);
      Serial.print(' '); 
      Serial.print(description);
      if (temperature !=0) {
        Serial.print(F(" ["));
        Serial.print((unsigned byte)temperature);
        Serial.print(F("] C"));
      }
      Serial.println();
      GB_Print::printEnd();
    }
  }
  

 static void printLogRecord(word address, word index, boolean events, boolean errors, boolean temperature){
    LogRecord logRecord;
    GB_Storage::read(address, &logRecord, sizeof(LogRecord));

    if (isEvent(logRecord) && !events){
      return;
    }
    if (isError(logRecord) && !errors){
      return;
    }
    if (isTemperature(logRecord) && !temperature){
      return;
    }

    Serial.print('#');
    Serial.print(index);
    Serial.print(' ');
    printLogRecord(logRecord);
  }

};

#endif
