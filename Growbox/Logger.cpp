#include "Logger.h"

#include "StorageHelper.h"

/////////////////////////////////////////////////////////////////////
//                             APPEND                              //
/////////////////////////////////////////////////////////////////////

// Normal event uses format [00DDDDDD]
//   00 - prefix for normal events 
//   DDDDDD - event identificator
void LoggerClass::logEvent(Event &event, byte value) {
  LogRecord logRecord(B00000000 | event.index, value);
  boolean isStored = GB_StorageHelper.storeLogRecord(logRecord);
  printLogRecordToSerialMonotior(logRecord, event.description, isStored);
}

// Watering event uses format [10SSDDDD]
//   10 - prefix for watering events 
//   SS - index of watering system [0..3]
//   DDDD - event identificator
void LoggerClass::logWateringEvent(byte wsIndex, WateringEvent& wateringEvent, byte value) {
  LogRecord logRecord(B10000000 | ((B00000011 & wsIndex) << 4) | (B00001111 & wateringEvent.index), value);
  boolean isStored = GB_StorageHelper.storeLogRecord(logRecord);
  printLogRecordToSerialMonotior(logRecord, wateringEvent.description, isStored);
}

// Error events uses format [01SSDDDD] 
//   01 - prefix for error events 
//   SS - length of errir seqence 
//   DDDD - sequence data
void LoggerClass::logError(Error &error) {
  LogRecord logRecord(B01000000 | ((B00000011 & (error.sequenceSize - 1)) << 4) | (B00001111 & error.sequence));
  error.isActive = true;
  boolean isStoredNow = false;
  if (!error.isStored) {
    error.isStored = GB_StorageHelper.storeLogRecord(logRecord);
    isStoredNow = error.isStored;
  }
  printLogRecordToSerialMonotior(logRecord, error.description, isStoredNow);
  //error.isStored = true;   
  error.notify();
}

boolean LoggerClass::stopLogError(Error &error) {
  if (error.isActive) {
    error.isActive = false;
    error.isStored = false;
    return true;
  }
  return false;
}

// Termometer events uses format [11TTTTTT].
//   11 - prefix for termometer events
//   TTTTTT - temperature [0..2^6] = [0..64]
void LoggerClass::logTemperature(byte temperature) {
  LogRecord logRecord(B11000000 | temperature);
  boolean isStored = GB_StorageHelper.storeLogRecord(logRecord);
  printLogRecordToSerialMonotior(logRecord, F("Temperature"), isStored, temperature);
}

/////////////////////////////////////////////////////////////////////
//                              CHECK                              //
/////////////////////////////////////////////////////////////////////

String LoggerClass::getLogRecordPrefix(const LogRecord &logRecord) {
  String out;
  out += '[';
  out += StringUtils::timeStampToString(logRecord.timeStamp);
  out += StringUtils::flashStringLoad(F("] ["));
  out += StringUtils::byteToHexString(logRecord.data, true);
  out += StringUtils::flashStringLoad(F("] "));
  return out;
}

const __FlashStringHelper* LoggerClass::getLogRecordDescription(LogRecord &logRecord) {

  if (logRecord.isEmpty()) {
    return F("Log record not loaded");
  }

  byte data = (logRecord.data & B00111111);
  if (isEvent(logRecord)) {
    Event* foundItemPtr = Event::findByKey(data);
    if (foundItemPtr == NULL) {
      return F("Unknown Event");
    }
    else {
      return foundItemPtr->description;
    }
  }
  byte wateringEventIndex = (data & B00001111);
  if (isWateringEvent(logRecord)) {
    WateringEvent* foundItemPtr = WateringEvent::findByKey(wateringEventIndex);
    if (foundItemPtr == NULL) {
      return F("Unknown Watering event");
    }
    else {
      return foundItemPtr->description;
    }
  }
  else if (isTemperature(logRecord)) {
    return F("Temperature");
  }
  else if (isError(logRecord)) {
    byte sequence = (data & B00001111);
    byte sequenceSize = ((data & B00110000)>>4) + 1;
    Error* foundItemPtr = Error::findByKey(sequence, sequenceSize);
    if (foundItemPtr == NULL) {
      return F("Unknown Error");
    }
    else {
      return foundItemPtr->description;
    }
  }
  else {
    return F("Unknown Log record");
  }
}

String LoggerClass::getLogRecordDescriptionSuffix(const LogRecord &logRecord, boolean formatForHtml) {

  String out;
  if (logRecord.isEmpty()) {
    return out;
  }
  if (isEvent(logRecord)) {
    if (logRecord.data == EVENT_FAN_ON_MIN.index || logRecord.data == EVENT_FAN_ON_MAX.index){
      if (logRecord.data1 != 0) {
        out += StringUtils::flashStringLoad(F(", "));
        out += ((B11110000 & logRecord.data1) >> 4) * UPDATE_GROWBOX_STATE_DELAY_MINUTES;
        out +='/';
        out += (B00001111 & logRecord.data1) * UPDATE_GROWBOX_STATE_DELAY_MINUTES;
        out += StringUtils::flashStringLoad(F(" min"));
      }
    }
  }else if (isWateringEvent(logRecord)) {
    byte wsIndex = ((logRecord.data & B00110000) >> 4);
    out += StringUtils::flashStringLoad(F(" system #"));
    out += (wsIndex + 1);

    byte wateringEventIndex = (logRecord.data & B00001111);
    WateringEvent* foundItemPtr = WateringEvent::findByKey(wateringEventIndex);

    if (foundItemPtr != NULL) {
      if (foundItemPtr->isData2Value) {
        out += StringUtils::flashStringLoad(F(" value ["));
        out += logRecord.data1;
        out += StringUtils::flashStringLoad(F("]"));
      }
      else if (foundItemPtr->isData2Duration) {
        out += StringUtils::flashStringLoad(F(" during "));
        out += logRecord.data1;
        out += StringUtils::flashStringLoad(F(" sec"));
      }
    }
  }
  else if (isTemperature(logRecord)) {
    byte temperature = (logRecord.data & B00111111);
    out += StringUtils::flashStringLoad(F(" "));
    out += temperature;
    if (formatForHtml) {
      out += StringUtils::flashStringLoad(F("&deg;C"));
    }
  }

  return out;
}

boolean LoggerClass::isEvent(const LogRecord &logRecord) {
  return !logRecord.isEmpty() && ((logRecord.data & B11000000) == B00000000);
}
boolean LoggerClass::isWateringEvent(const LogRecord &logRecord) {
  return !logRecord.isEmpty() && ((logRecord.data & B11000000) == B10000000);
}
boolean LoggerClass::isError(const LogRecord &logRecord) {
  return !logRecord.isEmpty() && ((logRecord.data & B11000000) == B01000000);
}
boolean LoggerClass::isTemperature(const LogRecord &logRecord) {
  return !logRecord.isEmpty() && ((logRecord.data & B11000000) == B11000000);
}

//private:

void LoggerClass::printLogRecordToSerialMonotior(const LogRecord &logRecord, const __FlashStringHelper* description, const boolean wasStored, const byte temperature) {
  if (!g_useSerialMonitor) {
    return;
  }
  Serial.print(F("LOG> "));
  if (!wasStored) {
    Serial.print(F("NOT STORED "));
  }
  Serial.print(getLogRecordPrefix(logRecord));
  Serial.print(description);
  Serial.print(getLogRecordDescriptionSuffix(logRecord, false));

  Serial.println();
}

LoggerClass GB_Logger;

