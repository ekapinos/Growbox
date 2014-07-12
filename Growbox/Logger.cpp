#include "Logger.h"

#include "StorageHelper.h"
#include "RAK410_XBeeWifi.h"


// Normal event uses uses format [00DDDDDD]
//   00 - prefix for normal events 
//   DDDDDD - event identificator
void LoggerClass::logEvent(Event &event){
  LogRecord logRecord(event.index);
  boolean isStored = GB_StorageHelper::storeLogRecord(logRecord);
  printDirtyLogRecord(logRecord, event.description, isStored);
}

// Error events uses format [01SSDDDD] 
//   01 - prefix for error events 
//   SS - length of errir seqence 
//   DDDD - sequence data
void LoggerClass::logError(Error &error){
  LogRecord logRecord(B01000000|(B00000011 | error.sequenceSize-1)<<4 | (B00001111 & error.sequence));
  boolean isStoredNow = false;
  if(!error.isStored){
    error.isStored = GB_StorageHelper::storeLogRecord(logRecord);
    isStoredNow = true;
  } 
  printDirtyLogRecord(logRecord, error.description, isStoredNow);
  error.isStored = true;   
  error.notify();
}
boolean LoggerClass::stopLogError(Error &error){
  if (error.isStored){
    error.isStored = false;
    return true;
  }
  return false;
}

// Termometer events uses format [11TTTTTT].
//   11 - prefix for termometer events
//   TTTTTT - temperature [0..2^6] = [0..64]
void LoggerClass::logTemperature(byte temperature){
  LogRecord logRecord(B11000000|temperature);
  boolean isStored = GB_StorageHelper::storeLogRecord(logRecord);
  printDirtyLogRecord(logRecord, FS(S_Temperature), isStored, temperature);
}

/////////////////////////////////////////////////////////////////////
//                        GROWBOX COMMANDS                         //
/////////////////////////////////////////////////////////////////////

int LoggerClass::getLogRecordsCount(){
  return GB_StorageHelper::getLogRecordsCount();
}  

LogRecord LoggerClass::getLogRecordByIndex(int index){
  LogRecord logRecord;
  GB_StorageHelper::getLogRecordByIndex(index, logRecord);
  return logRecord;
}

String LoggerClass::getLogRecordPrefix(const LogRecord &logRecord){        
  String out;
  out += StringUtils::getTimeString(logRecord.timeStamp);
  out += ' '; 
  out += StringUtils::getHEX(logRecord.data, true);
  out += ' '; 
  return out;
}

const __FlashStringHelper* LoggerClass::getLogRecordDescription(LogRecord &logRecord) {
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
    return FS(S_Temperature);
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

String LoggerClass::getLogRecordSuffix(const LogRecord &logRecord){        
  String out;
  if (isTemperature(logRecord)) {
    byte temperature = (logRecord.data & B00111111);
    out += StringUtils::flashStringLoad(F(" ["));
    out += temperature;
    out += StringUtils::flashStringLoad(F("] C"));
  }
  //Serial.print(F(" HEX: "));
  //GB_PrintDirty::printRAM(&((LogRecord)logRecord), sizeof(LogRecord));  

  return out;
}

boolean LoggerClass::isEvent(const LogRecord &logRecord){
  return (logRecord.data & B11000000) == B00000000;
}
boolean LoggerClass::isError(const LogRecord &logRecord){
  return (logRecord.data & B11000000) == B01000000;
}
boolean LoggerClass::isTemperature(const LogRecord &logRecord){
  return (logRecord.data & B11000000) == B11000000;
}


//private:

void LoggerClass::printDirtyLogRecord(const LogRecord &logRecord, const __FlashStringHelper* description, const boolean isStored, const byte temperature){
  if (!RAK410_XBeeWifi.useSerialMonitor) {
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

LoggerClass GB_Logger;

