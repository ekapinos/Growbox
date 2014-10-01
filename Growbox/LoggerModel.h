#ifndef LoggerModel_h
#define LoggerModel_h

#include "Global.h"

/////////////////////////////////////////////////////////////////////
//                               EVENT                             //
/////////////////////////////////////////////////////////////////////

class Event{
private:
  static LinkedList<Event> fullList;

public:
  byte index;
  const __FlashStringHelper* description; // FLASH

  Event();

  void init(byte index, const __FlashStringHelper* description);

  static Event* findByKey(byte index);

  static boolean isInitialized();

};

/////////////////////////////////////////////////////////////////////
//                           WATERING EVENT                        //
/////////////////////////////////////////////////////////////////////

class WateringEvent{
private:
  static LinkedList<WateringEvent> fullList;

public:
  byte index;
  const __FlashStringHelper* description; // FLASH  
  const __FlashStringHelper* shortDescription;// FLASH
  boolean isData2Value;
  boolean isData2Duration;

  WateringEvent();

  void init(byte index, const __FlashStringHelper* description, const __FlashStringHelper* shortDescription, boolean isData2Value, boolean isData2Duration);

  static WateringEvent* findByKey(byte index);

  static boolean isInitialized();

};

/////////////////////////////////////////////////////////////////////
//                               ERROR                             //
/////////////////////////////////////////////////////////////////////

class Error{
private:
  static LinkedList<Error> fullList;

public:
  // sequence - human readble sequence of signals, e.g.[B010] means [short, long, short]
  // sequenceSize - byte in region 1..4
  byte sequence;
  byte sequenceSize;
  const __FlashStringHelper* description; // FLASH
  boolean isActive;// should be stored in Log only once, but notification should repeated
  boolean isStored;

  Error();

  void init(byte sequence, byte sequenceSize, const __FlashStringHelper* description);

  static Error* findByKey(byte sequence, byte sequenceSize);

  static boolean isInitialized();

  void notify();

};

extern Event EVENT_FIRST_START_UP, EVENT_RESTART, EVENT_MODE_DAY,
    EVENT_MODE_NIGHT, EVENT_LIGHT_OFF, EVENT_LIGHT_ON, EVENT_LIGHT_ENABLED,
    EVENT_LIGHT_DISABLED, EVENT_FAN_OFF, EVENT_FAN_ON_LOW, EVENT_FAN_ON_HIGH,
    EVENT_FAN_ENABLED, EVENT_FAN_DISABLED,
    EVENT_LOGGER_ENABLED, EVENT_LOGGER_DISABLED, EVENT_CLOCK_AUTO_ADJUST,
    EVENT_HEATER_OFF, EVENT_HEATER_ON, EVENT_HEATER_ENABLED, EVENT_HEATER_DISABLED,
    EVENT_THERMOMETER_RESTORED;

//16 elements -  max
extern WateringEvent WATERING_EVENT_WET_SENSOR_IN_AIR, WATERING_EVENT_WET_SENSOR_VERY_DRY,
    WATERING_EVENT_WET_SENSOR_DRY, WATERING_EVENT_WET_SENSOR_NORMAL,
    WATERING_EVENT_WET_SENSOR_WET, WATERING_EVENT_WET_SENSOR_VERY_WET,
    WATERING_EVENT_WET_SENSOR_SHORT_CIRCIT, WATERING_EVENT_WET_SENSOR_UNSTABLE,
    WATERING_EVENT_WET_SENSOR_DISABLED,

    WATERING_EVENT_WATER_PUMP_ON_DRY, WATERING_EVENT_WATER_PUMP_ON_VERY_DRY,
    WATERING_EVENT_WATER_PUMP_ON_NO_SENSOR_DRY,
    WATERING_EVENT_WATER_PUMP_ON_MANUAL_DRY,
    WATERING_EVENT_WATER_PUMP_ON_AUTO_DRY, WATERING_EVENT_WATER_PUMP_OFF;

extern Error ERROR_CLOCK_NOT_SET, ERROR_CLOCK_NEEDS_SYNC,
    ERROR_TERMOMETER_DISCONNECTED, ERROR_TERMOMETER_ZERO_VALUE,
    ERROR_TERMOMETER_CRITICAL_VALUE, ERROR_MEMORY_LOW,
    ERROR_AT24C32_EEPROM_DISCONNECTED, ERROR_CLOCK_RTC_DISCONNECTED;


void initLoggerModel();

#endif

