#ifndef GB_Loggger_h
#define GB_Loggger_h

#include "LoggerModel.h"
#include "StorageHelper.h"
#include "SerialHelper.h"
#include "Print.h"

class GB_Logger {

public:

  /////////////////////////////////////////////////////////////////////
  //                             WRITE                               //
  /////////////////////////////////////////////////////////////////////

  // Normal event uses uses format [00DDDDDD]
  //   00 - prefix for normal events 
  //   DDDDDD - event identificator
  static void logEvent(Event &event){
    LogRecord logRecord(event.index);
    boolean isStored = GB_StorageHelper::storeLogRecord(logRecord);
    processLogRecord(logRecord, event.description, isStored);
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
    processLogRecord(logRecord, error.description, error.isStored);
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
    processLogRecord(logRecord, F("Temperature"), isStored, temperature);
  }

  static void printLogRecord(LogRecord &logRecord) {

    byte data = (logRecord.data & B00111111);

    if (isEvent(logRecord)){
      Event* foundItemPtr = Event::findByIndex(data);
      if (foundItemPtr == 0){
        processLogRecord(logRecord, F("Unknown event"), false);
      } 
      else {
        processLogRecord(logRecord, foundItemPtr->description, false);
      }

    } 
    else if (isTemperature(logRecord)){
      processLogRecord(logRecord, F("Temperature"), false, data); 

    } 
    else if (isError(logRecord)){    
      byte sequence = (data & B00001111); 
      byte sequenceSize = (data & B00110000)>>4; 
      Error* foundItemPtr = Error::findByIndex(sequence, sequenceSize);
      if (foundItemPtr == 0){
        processLogRecord(logRecord, F("Unknown error"), false);
      } 
      else {
        processLogRecord(logRecord, foundItemPtr->description, false);
      }
    } 
    else {
      processLogRecord(logRecord, F("Unknown"), false);
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

  /////////////////////////////////////////////////////////////////////
  //                        GROWBOX COMMANDS                         //
  /////////////////////////////////////////////////////////////////////


  static void printFullLog(boolean printEvents, boolean printErrors, boolean printTemperature){
    LogRecord logRecord;
    boolean isEmpty = true;
    ;
    for (int i = 0; i<GB_StorageHelper::getLogRecordsCount(); i++){
      if (GB_StorageHelper::getLogRecord(i, logRecord)){


        if (isEvent(logRecord) && !printEvents){
          continue;
        }
        if (isError(logRecord) && !printErrors){
          continue;
        }
        if (isTemperature(logRecord) && !printTemperature){
          continue;
        }

        Serial.print('#');
        Serial.print(i);
        Serial.print(' ');
        printLogRecord(logRecord);


        isEmpty = false;
      } 
      else {
        // TODO check it
      }  
    }

    if (isEmpty){
      Serial.println(F("- no records in log"));
    }
    // TODO check Serial
  }


  // TODO improuve

  void setLoggerEnable(boolean flag){
    GB_StorageHelper::setLoggerEnabled(flag);
  }

private:

  static void processLogRecord(const LogRecord &logRecord, const __FlashStringHelper* description, boolean isStored, byte temperature = 0){

    if (g_UseSerialMonitor) {
      if (!isStored) {
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
      GB_SerialHelper::printEnd();
    }
  }


};

#endif


