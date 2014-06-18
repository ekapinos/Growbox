#ifndef GB_Error_h
#define GB_Error_h

#include "Global.h"

// error blinks in milleseconds and blink sequences
const word ERROR_SHORT_SIGNAL_MS = 100;  // -> 0
const word ERROR_LONG_SIGNAL_MS = 400;   // -> 1
const word ERROR_DELAY_BETWEEN_SIGNALS_MS = 150;

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

extern Error 

ERROR_TIMER_NOT_SET,
ERROR_TIMER_NEEDS_SYNC,
ERROR_TERMOMETER_DISCONNECTED,
ERROR_TERMOMETER_ZERO_VALUE,
ERROR_TERMOMETER_CRITICAL_VALUE,
ERROR_MEMORY_LOW;

void initErrors();

#endif

