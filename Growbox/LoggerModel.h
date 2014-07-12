#ifndef LoggerModel_h
#define LoggerModel_h

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

  Error();

  void init(byte sequence, byte sequenceSize, const __FlashStringHelper* description) ;

  static Error* findByIndex(byte sequence, byte sequenceSize);

  static boolean isInitialized();

  void notify() ;

};

class Event {
private:
  static Event* lastAddedEvent;
  const Event* nextEvent;

public:
  byte index;
  const __FlashStringHelper* description; // FLASH

  Event();

  void init(byte index, const __FlashStringHelper* description) ;

  static Event* findByIndex(byte index);

  static boolean isInitialized();

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

void initLoggerModel(); // TODO move ?

#endif



