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

boolean  Event::isInitialized(){
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
EVENT_FAN_ON_MAX;//,
//EVENT_SERIAL_UNKNOWN_COMMAND;

void initLoggerModel(){

  // Use F macro to reduce requirements to memory. We can't use F macro in constructors.
  ERROR_TIMER_NOT_SET.init(B00, 2, F("Error: Timer not set"));
  ERROR_TIMER_NEEDS_SYNC.init(B001, 3, F("Error: Timer needs sync"));
  ERROR_TERMOMETER_DISCONNECTED.init(B01, 2, F("Error: Termometer disconnected"));
  ERROR_TERMOMETER_ZERO_VALUE.init(B010, 3, F("Error: Termometer returned ZERO value"));
  ERROR_TERMOMETER_CRITICAL_VALUE.init(B000, 3, F("Error: Termometer returned CRITICAL value"));
  ERROR_MEMORY_LOW.init(B111, 3, F("Error: Memory remained less 200 bytes"));

  EVENT_FIRST_START_UP.init(1, F("First startup")), 
  EVENT_RESTART.init(2, F("Restart")), 
  EVENT_MODE_DAY.init(3, F("DAY mode")), 
  EVENT_MODE_NIGHT.init(4, F("NIGHT mode")), 
  EVENT_LIGHT_OFF.init(5, F("Light OFF")), 
  EVENT_LIGHT_ON.init(6, F("Light ON")), 
  EVENT_FAN_OFF.init(7, F("Fan OFF")), 
  EVENT_FAN_ON_MIN.init(8, F("Fan ON, MIN speed")), 
  EVENT_FAN_ON_MAX.init(9, F("Fan ON, MAX speed"));//,
  //EVENT_SERIAL_UNKNOWN_COMMAND.init(10, F("Unknown serial command"));
}





