#ifndef LoggerModel_h
#define LoggerModel_h

#include "Global.h"

/////////////////////////////////////////////////////////////////////
//                               ERROR                             //
/////////////////////////////////////////////////////////////////////

class Error {
private:
  static LinkedList<Error> fullList;

public:
  // sequence - human readble sequence of signals, e.g.[B010] means [short, long, short] 
  // sequenceSize - byte in region 1..4
  byte sequence; 
  byte sequenceSize;
  const __FlashStringHelper* description; // FLASH
  boolean isStored; // should be stored in Log only once, but notification should repeated

  Error();

  void init(byte sequence, byte sequenceSize, const __FlashStringHelper* description) ;

  static Error* findByKey(byte sequence, byte sequenceSize);

  static boolean isInitialized();

  void notify() ;

};


/////////////////////////////////////////////////////////////////////
//                               EVENT                             //
/////////////////////////////////////////////////////////////////////

class Event {
private:
  static LinkedList<Event> fullList;

public:
  byte index;
  const __FlashStringHelper* description; // FLASH

  Event();

  void init(byte index, const __FlashStringHelper* description) ;

  static Event* findByKey(byte index);

  static boolean isInitialized();

};

extern Error 
ERROR_TIMER_NOT_SET,
ERROR_TIMER_NEEDS_SYNC,
ERROR_TERMOMETER_DISCONNECTED,
ERROR_TERMOMETER_ZERO_VALUE,
ERROR_TERMOMETER_CRITICAL_VALUE,
ERROR_MEMORY_LOW;
/*EVENT_DATA_CHECK_PERIOD_SEC,
 EVENT_DATA_TEMPERATURE_DAY,
 EVENT_DATA_TEMPERATURE_NIGHT,
 EVENT_DATA_TEMPERATURE_CRITICAL,
 EVENT_DATA_TEMPERATURE_DELTA,*/
//EVENT_RTC_SYNKC(10, "RTC synced");

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

void initLoggerModel();

#endif




