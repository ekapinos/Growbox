#ifndef GB_LogModel_h
#define GB_LogModel_h

#include "Global.h"

class Error {
private:
  static Error* lastAddedItem;
  const Error* nextError;

public:
  // sequence - human readble sequence of signals, e.g.[B010] means [short, long, short] 
  // sequenceSize - byte in region 1..4
  byte sequence; 
  byte sequenceSize;
  const __FlashStringHelper* description; // FLASH
  boolean isStored; // should be stored in Log only once, but notification should repeated

    Error() : 
  nextError(lastAddedItem), sequence(0xFF), sequenceSize(0xFF), isStored(false) {
    lastAddedItem = this;
  }

  void init(byte sequence, byte sequenceSize, const __FlashStringHelper* description) {
    this->sequence = sequence;
    this->sequenceSize = sequenceSize;
    this->description = description;
  }
  
  static Error* findByIndex(byte sequence, byte sequenceSize){
    Error* currentItemPtr = lastAddedItem;
    while (currentItemPtr != 0){
      if (currentItemPtr->sequence == sequence && currentItemPtr->sequenceSize == sequenceSize) {
        break;
      }
      currentItemPtr = (Error*)currentItemPtr->nextError;
    }
    return 0;
  }

  static boolean isInitialized(){
    return (findByIndex(0xFF, 0xFF) == 0);
  }
  
  
void notify() {
  digitalWrite(ERROR_PIN, LOW);
  delay(1000);
  for (int i = sequenceSize-1; i >= 0; i--){
    digitalWrite(ERROR_PIN, HIGH);
    if (bitRead(sequence, i)){
      delay(ERROR_LONG_SIGNAL_MS);
    } 
    else {
      delay(ERROR_SHORT_SIGNAL_MS);
    } 
    digitalWrite(ERROR_PIN, LOW);
    delay(ERROR_DELAY_BETWEEN_SIGNALS_MS);
  }
  digitalWrite(ERROR_PIN, LOW);
  delay(1000);
}

};

class Event {
private:
  static Event* lastAddedEvent;
  const Event* nextEvent;

public:
  byte index;
  const __FlashStringHelper* description; // FLASH

  Event() : 
  nextEvent(lastAddedEvent), index(0xFF) {
    lastAddedEvent = this;
  }

  void init(byte index, const __FlashStringHelper* description) {
    this->index = index;
    this->description = description;
  }

  static Event* findByIndex(byte index){
    Event* currentItemPtr = lastAddedEvent;
    while (currentItemPtr != 0){
      if (currentItemPtr->index == index) {
        break;
      }
      currentItemPtr = (Event*)currentItemPtr->nextEvent;
    }
    return 0;
  }

  static boolean isInitialized(){
    return (findByIndex(0xFF) == 0);
  }

};

extern Error 
ERROR_TIMER_NOT_SET,
ERROR_TIMER_NEEDS_SYNC,
ERROR_TERMOMETER_DISCONNECTED,
ERROR_TERMOMETER_ZERO_VALUE,
ERROR_TERMOMETER_CRITICAL_VALUE,
ERROR_MEMORY_LOW;

extern Event
EVENT_FIRST_START_UP, 
EVENT_RESTART, 
EVENT_MODE_DAY, 
EVENT_MODE_NIGHT, 
EVENT_LIGHT_OFF, 
EVENT_LIGHT_ON, 
EVENT_FAN_OFF, 
EVENT_FAN_ON_MIN, 
EVENT_FAN_ON_MAX,
EVENT_SERIAL_UNKNOWN_COMMAND;

void initErrors();
void initEvents();

#endif

