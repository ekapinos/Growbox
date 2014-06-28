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
    boolean isStoredNow = false;
    if(!error.isStored){
      error.isStored = GB_StorageHelper::storeLogRecord(logRecord);
      isStoredNow = true;
    } 
    printDirtyLogRecord(logRecord, error.description, isStoredNow);
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

  static int getLogRecordsCount(){
    return GB_StorageHelper::getLogRecordsCount();
  }  

  static LogRecord getLogRecordByIndex(int index){
    LogRecord logRecord;
    GB_StorageHelper::getLogRecordByIndex(index, logRecord);
    return logRecord;
  }

  static String getLogRecordPrefix(const LogRecord &logRecord){        
    String out;
    out += GB_PrintDirty::getTimeString(logRecord.timeStamp);
    out += ' '; 
    out += GB_PrintDirty::getHEX(logRecord.data, true);
    out += ' '; 
    return out;
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
  
  static String getLogRecordSuffix(const LogRecord &logRecord){        
    String out;
    if (isTemperature(logRecord)) {
      byte temperature = (logRecord.data & B00111111);
      out += " [";
      out += temperature;
      out += "] C";
    }
    //Serial.print(F(" HEX: "));
    //GB_PrintDirty::printRAM(&((LogRecord)logRecord), sizeof(LogRecord));  

    return out;
  }

  static boolean isEvent(const LogRecord &logRecord){
    return (logRecord.data & B11000000) == B00000000;
  }
  static boolean isError(const LogRecord &logRecord){
    return (logRecord.data & B11000000) == B01000000;
  }
  static boolean isTemperature(const LogRecord &logRecord){
    return (logRecord.data & B11000000) == B11000000;
  }


private:

  static void printDirtyLogRecord(const LogRecord &logRecord, const __FlashStringHelper* description, const boolean isStored, const byte temperature = 0xFF){
    if (!GB_SerialHelper::useSerialMonitor) {
      return;
    }
    Serial.print(F("LOG> ")); 
    if (!isStored) {
      Serial.print(F("NOT STORED "));
    }
    Serial.print(getLogRecordPrefix(logRecord));    
    Serial.print(description);
    Serial.print(getLogRecordSuffix(logRecord));  

    Serial.println();      
  }

};

#endif












