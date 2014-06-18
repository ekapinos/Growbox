#ifndef GB_Event_h
#define GB_Event_h

#include "Global.h"

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

void initEvents();

#endif
