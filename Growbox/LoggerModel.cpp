#include "LoggerModel.h"

/////////////////////////////////////////////////////////////////////
//                               ERROR                             //
/////////////////////////////////////////////////////////////////////

LinkedList<Error> Error::fullList = LinkedList<Error>();

Error::Error() :
    sequence(0xFF), sequenceSize(0xFF), isActive(false), isStored(false), description(NULL){
  fullList.add(this);
}

void Error::init(byte sequence, byte sequenceSize, const __FlashStringHelper* description) {
  this->sequence = sequence;
  this->sequenceSize = sequenceSize;
  this->description = description;
}

Error* Error::findByKey(byte sequence, byte sequenceSize) {

  Iterator<Error> iterator = fullList.getIterator();
  while (iterator.hasNext()) {
    Error* currentItemPtr = iterator.getNext();
    if (currentItemPtr->sequence == sequence && currentItemPtr->sequenceSize == sequenceSize) {
      return currentItemPtr;
    }
  }
  return NULL;
}

boolean Error::isInitialized() {
  return (findByKey(0xFF, 0xFF) == 0);
}

void Error::notify() {
  digitalWrite(ERROR_PIN, LOW);
  delay(1000);
  for (int i = sequenceSize - 1; i >= 0; i--) {
    digitalWrite(ERROR_PIN, HIGH);
    if (bitRead(sequence, i)) {
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

/////////////////////////////////////////////////////////////////////
//                               EVENT                             //
/////////////////////////////////////////////////////////////////////

LinkedList<Event> Event::fullList = LinkedList<Event>();

Event::Event() :
    index(0xFF), description(NULL) {
  fullList.add(this);
}

void Event::init(byte index, const __FlashStringHelper* description) {
  this->index = index;
  this->description = description;
}

Event* Event::findByKey(byte index) {
  Iterator<Event> iterator = fullList.getIterator();
  while (iterator.hasNext()) {
    Event* currentItemPtr = iterator.getNext();
    if (currentItemPtr->index == index) {
      return currentItemPtr;
    }
  }
  return NULL;
}

boolean Event::isInitialized() {
  return (findByKey(0xFF) == NULL);
}

/////////////////////////////////////////////////////////////////////
//                          WATERING EVENT                         //
/////////////////////////////////////////////////////////////////////

LinkedList<WateringEvent> WateringEvent::fullList = LinkedList<WateringEvent>();

WateringEvent::WateringEvent() :
    index(0xFF), description(NULL), shortDescription(NULL), isData2Value(false), isData2Duration(false) {
  fullList.add(this);
}

void WateringEvent::init(byte index, const __FlashStringHelper* description, const __FlashStringHelper* shortDescription, boolean isData2Value, boolean isData2Duration) {
  this->index = index;
  this->description = description;
  this->shortDescription = shortDescription;
  this->isData2Value = isData2Value;
  this->isData2Duration = isData2Duration;
}

WateringEvent* WateringEvent::findByKey(byte index) {
  Iterator<WateringEvent> iterator = fullList.getIterator();
  while (iterator.hasNext()) {
    WateringEvent* currentItemPtr = iterator.getNext();
    if (currentItemPtr->index == index) {
      return currentItemPtr;
    }
  }
  return NULL;
}

boolean WateringEvent::isInitialized() {
  return (findByKey(0xFF) == NULL);
}

Error ERROR_CLOCK_NOT_SET, ERROR_CLOCK_NEEDS_SYNC,
    ERROR_TERMOMETER_DISCONNECTED, ERROR_TERMOMETER_ZERO_VALUE,
    ERROR_MEMORY_LOW,
    ERROR_AT24C32_EEPROM_DISCONNECTED, ERROR_CLOCK_RTC_DISCONNECTED;

Event EVENT_FIRST_START_UP, EVENT_RESTART, EVENT_MODE_DAY, EVENT_MODE_NIGHT,
    EVENT_LIGHT_OFF, EVENT_LIGHT_ON, EVENT_LIGHT_ENABLED, EVENT_LIGHT_DISABLED,
    EVENT_FAN_OFF, EVENT_FAN_ON_LOW, EVENT_FAN_ON_HIGH, EVENT_FAN_ENABLED,
    EVENT_FAN_DISABLED, EVENT_LOGGER_ENABLED, EVENT_LOGGER_DISABLED,
    EVENT_CLOCK_AUTO_ADJUST, EVENT_HEATER_OFF, EVENT_HEATER_ON, EVENT_HEATER_ENABLED, EVENT_HEATER_DISABLED;

WateringEvent //16 elements -  max
WATERING_EVENT_WET_SENSOR_IN_AIR, WATERING_EVENT_WET_SENSOR_VERY_DRY,
    WATERING_EVENT_WET_SENSOR_DRY, WATERING_EVENT_WET_SENSOR_NORMAL,
    WATERING_EVENT_WET_SENSOR_WET, WATERING_EVENT_WET_SENSOR_VERY_WET,
    WATERING_EVENT_WET_SENSOR_SHORT_CIRCIT, WATERING_EVENT_WET_SENSOR_UNSTABLE,
    WATERING_EVENT_WET_SENSOR_DISABLED,

    WATERING_EVENT_WATER_PUMP_ON_DRY, WATERING_EVENT_WATER_PUMP_ON_VERY_DRY,
    WATERING_EVENT_WATER_PUMP_ON_NO_SENSOR_DRY,
    WATERING_EVENT_WATER_PUMP_ON_MANUAL_DRY,
    WATERING_EVENT_WATER_PUMP_ON_AUTO_DRY, WATERING_EVENT_WATER_PUMP_OFF;

void initLoggerModel() {

  // Use F macro to reduce requirements to memory. We can't use F macro in constructors.
  ERROR_CLOCK_NOT_SET.init(B00, 2, F("Error: Clock not set"));
  ERROR_CLOCK_NEEDS_SYNC.init(B01, 2, F("Error: Clock needs sync"));
  ERROR_TERMOMETER_DISCONNECTED.init(B000, 3, F("Error: Thermometer disconnected"));
  ERROR_TERMOMETER_ZERO_VALUE.init(B001, 3, F("Error: Thermometer returned ZERO value"));
  ERROR_MEMORY_LOW.init(B011, 3, F("Error: Free memory less than 200 bytes"));
  ERROR_AT24C32_EEPROM_DISCONNECTED.init(B100, 3, F("Error: External AT24C32 EEPROM disconnected"));
  ERROR_CLOCK_RTC_DISCONNECTED.init(B101, 3, F("Error: Real-time clock disconnected"));

  // Zero index used for Empty Log Records, do not use it for Events
  EVENT_FIRST_START_UP.init(1, F("First startup"));
  EVENT_RESTART.init(2, F("Restart"));
  EVENT_MODE_DAY.init(3, F("Mode day"));
  EVENT_MODE_NIGHT.init(4, F("Mode night"));
  EVENT_LIGHT_OFF.init(5, F("Light off"));
  EVENT_LIGHT_ON.init(6, F("Light on"));
  EVENT_LIGHT_ENABLED.init(7, F("Light enabled"));
  EVENT_LIGHT_DISABLED.init(8, F("Light disabled"));
  EVENT_FAN_OFF.init(9, F("Fan off"));
  EVENT_FAN_ON_LOW.init(10, F("Fan low speed"));
  EVENT_FAN_ON_HIGH.init(11, F("Fan high speed"));
  EVENT_FAN_ENABLED.init(12, F("Fan enabled"));
  EVENT_FAN_DISABLED.init(13, F("Fan disabled"));
  EVENT_LOGGER_ENABLED.init(14, F("Logger enabled"));
  EVENT_LOGGER_DISABLED.init(15, F("Logger disabled"));
  EVENT_CLOCK_AUTO_ADJUST.init(16, F("Clock auto adjust"));
  EVENT_HEATER_OFF.init(17, F("Heater off"));;
  EVENT_HEATER_ON.init(18, F("Heater on"));;
  EVENT_HEATER_ENABLED.init(19, F("Heater enabled"));;
  EVENT_HEATER_DISABLED.init(20, F("Heater disabled"));;

  // 0..15 (max)
  WATERING_EVENT_WET_SENSOR_IN_AIR.init(1, F("Wet sensor [In Air]"), F("In Air"), true, false);
  WATERING_EVENT_WET_SENSOR_VERY_DRY.init(2, F("Wet sensor [Very Dry]"), F("Very Dry"), true, false);
  WATERING_EVENT_WET_SENSOR_DRY.init(3, F("Wet sensor [Dry]"), F("Dry"), true, false);
  WATERING_EVENT_WET_SENSOR_NORMAL.init(4, F("Wet sensor [Normal]"), F("Normal"), true, false);
  WATERING_EVENT_WET_SENSOR_WET.init(5, F("Wet sensor [Wet]"), F("Wet"), true, false);
  WATERING_EVENT_WET_SENSOR_VERY_WET.init(6, F("Wet sensor [Very Wet]"), F("Very Wet"), true, false);
  WATERING_EVENT_WET_SENSOR_SHORT_CIRCIT.init(7, F("Wet sensor [Short Circit]"), F("Short Circit"), true, false);
  WATERING_EVENT_WET_SENSOR_UNSTABLE.init(8, F("Wet sensor [Unstable]"), F("Unstable"), false, false);
  WATERING_EVENT_WET_SENSOR_DISABLED.init(9, F("Wet sensor [Disabled]"), F("Disabled"), false, false);

  WATERING_EVENT_WATER_PUMP_ON_DRY.init(10, F("Pump on [Sensor ok, Dry watering]"), F("Sensor ok, Dry watering"), false, true);
  WATERING_EVENT_WATER_PUMP_ON_VERY_DRY.init(11, F("Pump on [Sensor ok, Very Dry watering]"), F("Sensor ok, Very Dry watering"), false, true);
  WATERING_EVENT_WATER_PUMP_ON_NO_SENSOR_DRY.init(12, F("Pump on [Sensor not ready, Dry watering]"), F("Sensor not ready, Dry watering"), false, true);
  WATERING_EVENT_WATER_PUMP_ON_MANUAL_DRY.init(13, F("Pump on [Manual, Dry watering]"), F("Manual, Dry watering"), false, true);
  WATERING_EVENT_WATER_PUMP_ON_AUTO_DRY.init(14, F("Pump on [Dry watering]"), F("Dry watering"), false, true);
  WATERING_EVENT_WATER_PUMP_OFF.init(15, F("Pump off"), F("Stop watering"), false, false);
}

