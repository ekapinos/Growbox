#include "LoggerModel.h"

/////////////////////////////////////////////////////////////////////
//                               ERROR                             //
/////////////////////////////////////////////////////////////////////

LinkedList<Error> Error::fullList = LinkedList<Error>();

Error::Error() : 
sequence(0xFF), sequenceSize(0xFF), isStored(false){
  fullList.add(this);
}

void Error::init(byte sequence, byte sequenceSize, const __FlashStringHelper* description) {
  this->sequence = sequence;
  this->sequenceSize = sequenceSize;
  this->description = description;
}

Error* Error::findByKey(byte sequence, byte sequenceSize){

  Iterator<Error> iterator = fullList.getIterator();
  while(iterator.hasNext()){
    Error* currentItemPtr = iterator.getNext();    
    if (currentItemPtr->sequence == sequence && currentItemPtr->sequenceSize == sequenceSize) {
      return currentItemPtr;
    }
  }
  return 0;
}

boolean Error::isInitialized(){
  return (findByKey(0xFF, 0xFF) == 0);
}

void Error::notify() {
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


/////////////////////////////////////////////////////////////////////
//                               EVENT                             //
/////////////////////////////////////////////////////////////////////

LinkedList<Event> Event::fullList = LinkedList<Event>();

Event::Event() : 
index(0xFF) {
  fullList.add(this);
}

void  Event::init(byte index, const __FlashStringHelper* description) {
  this->index = index;
  this->description = description;
}

Event* Event::findByKey(byte index){
  Iterator<Event> iterator = fullList.getIterator();
  while(iterator.hasNext()){
    Event* currentItemPtr = iterator.getNext();    
    if (currentItemPtr->index == index) {
      return currentItemPtr;
    }
  }
  return 0;
}

boolean Event::isInitialized(){
  return (findByKey(0xFF) == 0);
}

/////////////////////////////////////////////////////////////////////
//                          WATERING EVENT                         //
/////////////////////////////////////////////////////////////////////

LinkedList<WateringEvent> WateringEvent::fullList = LinkedList<WateringEvent>();

WateringEvent::WateringEvent() : 
index(0xFF) {
  fullList.add(this);
}

void  WateringEvent::init(byte index, const __FlashStringHelper* description, const __FlashStringHelper* shortDescription) {
  this->index = index;
  this->description = description; 
  this->shortDescription = shortDescription;
}

WateringEvent* WateringEvent::findByKey(byte index){
  Iterator<WateringEvent> iterator = fullList.getIterator();
  while(iterator.hasNext()){
    WateringEvent* currentItemPtr = iterator.getNext();    
    if (currentItemPtr->index == index) {
      return currentItemPtr;
    }
  }
  return 0;
}

boolean WateringEvent::isInitialized(){
  return (findByKey(0xFF) == 0);
}


Error 
ERROR_TIMER_NOT_SET,
ERROR_TIMER_NEEDS_SYNC,
ERROR_TERMOMETER_DISCONNECTED,
ERROR_TERMOMETER_ZERO_VALUE,
ERROR_TERMOMETER_CRITICAL_VALUE,
ERROR_MEMORY_LOW;

Event 
EVENT_FIRST_START_UP, 
EVENT_RESTART, 
EVENT_MODE_DAY, 
EVENT_MODE_NIGHT, 
EVENT_LIGHT_OFF, 
EVENT_LIGHT_ON, 
EVENT_FAN_OFF, 
EVENT_FAN_ON_MIN, 
EVENT_FAN_ON_MAX,
EVENT_LOGGER_ENABLED,
EVENT_LOGGER_DISABLED;

WateringEvent //16 elements -  max
WATERING_EVENT_WET_SENSOR_IN_AIR,
WATERING_EVENT_WET_SENSOR_VERY_DRY,
WATERING_EVENT_WET_SENSOR_DRY,
WATERING_EVENT_WET_SENSOR_NORMAL,
WATERING_EVENT_WET_SENSOR_WET,
WATERING_EVENT_WET_SENSOR_VERY_WET,
WATERING_EVENT_WET_SENSOR_SHORT_CIRCIT,
WATERING_EVENT_WET_SENSOR_UNSTABLE,
WATERING_EVENT_WET_SENSOR_DISABLED,
 
WATERING_EVENT_WATER_PUMP_ON_DRY,
WATERING_EVENT_WATER_PUMP_ON_VERY_DRY,
WATERING_EVENT_WATER_PUMP_AUTO_ON_DRY;
   
void initLoggerModel(){

  // Use F macro to reduce requirements to memory. We can't use F macro in constructors.
  ERROR_TIMER_NOT_SET.init(B00, 2, F("Error: Timer not set"));
  ERROR_TIMER_NEEDS_SYNC.init(B001, 3, F("Error: Timer needs sync"));
  ERROR_TERMOMETER_DISCONNECTED.init(B01, 2, F("Error: Termometer disconnected"));
  ERROR_TERMOMETER_ZERO_VALUE.init(B010, 3, F("Error: Termometer returned ZERO value"));
  ERROR_TERMOMETER_CRITICAL_VALUE.init(B000, 3, F("Error: Termometer returned CRITICAL value"));
  ERROR_MEMORY_LOW.init(B111, 3, F("Error: Memory remained less 200 bytes"));

  EVENT_FIRST_START_UP.init(1, F("First startup"));
  EVENT_RESTART.init(2, F("Restart"));
  EVENT_MODE_DAY.init(3, F("Day mode"));
  EVENT_MODE_NIGHT.init(4, F("Night mode"));
  EVENT_LIGHT_OFF.init(5, F("Light off"));
  EVENT_LIGHT_ON.init(6, F("Light on"));
  EVENT_FAN_OFF.init(7, F("Fan off"));
  EVENT_FAN_ON_MIN.init(8, F("Fan min speed")); 
  EVENT_FAN_ON_MAX.init(9, F("Fan max speed"));
  EVENT_LOGGER_ENABLED.init(10, F("Logger enabled"));
  EVENT_LOGGER_DISABLED.init(11, F("Logger disabled"));
  
  // 0..15 (max)
  WATERING_EVENT_WET_SENSOR_IN_AIR.init(1, F("Wet sensor [In Air]"), F("In Air"));
  WATERING_EVENT_WET_SENSOR_VERY_DRY.init(2, F("Wet sensor [Very Dry]"), F("Very Dry"));
  WATERING_EVENT_WET_SENSOR_DRY.init(3, F("Wet sensor [Dry]"), F("Dry"));
  WATERING_EVENT_WET_SENSOR_NORMAL.init(4, F("Wet sensor [Normal]"), F("Normal"));
  WATERING_EVENT_WET_SENSOR_WET.init(5, F("Wet sensor [Wet]"), F("Wet"));
  WATERING_EVENT_WET_SENSOR_VERY_WET.init(6, F("Wet sensor [Very Wet]"), F("Very Wet"));
  WATERING_EVENT_WET_SENSOR_SHORT_CIRCIT.init(7, F("Wet sensor [Short Circit]"), F("Short Circit"));
  WATERING_EVENT_WET_SENSOR_UNSTABLE.init(8, F("Wet sensor [Unstable]"), F("Unstable"));
  WATERING_EVENT_WET_SENSOR_DISABLED.init(9, F("Wet sensor [Disabled]"), F("Disabled"));
   
  WATERING_EVENT_WATER_PUMP_ON_DRY.init(13, F("Pump [Dry watering]"), F("Dry watering"));
  WATERING_EVENT_WATER_PUMP_ON_VERY_DRY.init(14, F("Pump [Very Dry watering]"), F("Very Dry watering"));
  WATERING_EVENT_WATER_PUMP_AUTO_ON_DRY.init(15, F("Pump [Auto Dry watering]"), F("Auto Dry watering"));
}






