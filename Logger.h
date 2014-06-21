#ifndef GB_Loggger_h
#define GB_Loggger_h

#include "LoggerModel.h"
#include "StorageHelper.h"
#include "SerialHelper.h"
#include "PrintDirty.h"

class GB_Logger {

public:

  // Normal event uses uses format [00DDDDDD]
  //   00 - prefix for normal events 
  //   DDDDDD - event identificator
  static void logEvent(Event &event){
    LogRecord logRecord(event.index);
    boolean isStored = GB_StorageHelper::storeLogRecord(logRecord);
    printDirtyLogRecord(logRecord, event.description, isStored);
    GB_SerialHelper::printDirtyEnd();
  }

  // Error events uses format [01SSDDDD] 
  //   01 - prefix for error events 
  //   SS - length of errir seqence 
  //   DDDD - sequence data
  static void logError(Error &error){
    LogRecord logRecord(B01000000|(B00000011 | error.sequenceSize-1)<<4 | (B00001111 & error.sequence));
    if(!error.isStored){
      error.isStored = GB_StorageHelper::storeLogRecord(logRecord);
    }
    printDirtyLogRecord(logRecord, error.description, error.isStored);
    GB_SerialHelper::printDirtyEnd();
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
    boolean isStored = GB_StorageHelper::storeLogRecord(logRecord);
    printDirtyLogRecord(logRecord, F("Temperature"), isStored, temperature);
    GB_SerialHelper::printDirtyEnd();
  }

  /////////////////////////////////////////////////////////////////////
  //                        GROWBOX COMMANDS                         //
  /////////////////////////////////////////////////////////////////////

  static void printFullLog(boolean printEvents, boolean printErrors, boolean printTemperature){
    LogRecord logRecord;
    boolean isEmpty = true;
    for (int i = 0; i<GB_StorageHelper::getLogRecordsCount(); i++){
      if (GB_StorageHelper::getLogRecordByIndex(i, logRecord)){
        if (!printEvents && isEvent(logRecord)){
          continue;
        }
        if (!printErrors && isError(logRecord)){
          continue;
        }
        if (!printTemperature && isTemperature(logRecord)){
          continue;
        }
        Serial.print('#');
        Serial.print(i);
        Serial.print(' ');
        const __FlashStringHelper* description = getLogRecordDescription(logRecord);
        if (isTemperature(logRecord)){
          byte temperature = (logRecord.data & B00111111);
          printDirtyLogRecord(logRecord, description, true, temperature);
        } 
        else {
          printDirtyLogRecord(logRecord, description, true);
        }
        isEmpty = false;
      } 
      else {
        // TODO check it
      }  
    }
    if (isEmpty){
      Serial.println(F("Log empty"));
    }
    // GB_SerialHelper::printEnd();  - no need it is Growbox command
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

  static const __FlashStringHelper* getLogRecordDescription(LogRecord &logRecord) {
    byte data = (logRecord.data & B00111111);   
    if (isEvent(logRecord)){
      Event* foundItemPtr = Event::findByIndex(data);
      if (foundItemPtr == 0){
        return F("Unknown event");
      } 
      else {
        return foundItemPtr->description;
      }
    } 
    else if (isTemperature(logRecord)){
      return F("Temperature");
    } 
    else if (isError(logRecord)){    
      byte sequence = (data & B00001111); 
      byte sequenceSize = (data & B00110000)>>4; 
      Error* foundItemPtr = Error::findByIndex(sequence, sequenceSize);
      if (foundItemPtr == 0){
        return F("Unknown error");
      } 
      else {
        return foundItemPtr->description;
      }
    } 
    else {
      return F("Unknown");
    }
  }

  static void printDirtyLogRecord(const LogRecord &logRecord, const __FlashStringHelper* description, boolean isStored, byte temperature = 0xFF){
    if (!GB_SerialHelper::useSerialMonitor) {
      return;
    }
    if (!isStored) {
      Serial.print(F("NOT STORED "));
    }
    GB_PrintDirty::printTime(logRecord.timeStamp); 
    Serial.print(F(" 0x")); 
    GB_PrintDirty::print2digitsHEX(logRecord.data);
    Serial.print(' '); 
    Serial.print(description);
    if (temperature != 0xFF) {
      Serial.print(F(" ["));
      Serial.print((unsigned byte)temperature);
      Serial.print(F("] C"));
    }
    //Serial.print(F(" HEX: "));
    //GB_PrintDirty::printRAM(&((LogRecord)logRecord), sizeof(LogRecord));
    Serial.println();      
  }

};

#endif









