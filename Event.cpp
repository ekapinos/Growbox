#include "Event.h"

Event* Event::lastAddedEvent = 0;

/*EVENT_DATA_CHECK_PERIOD_SEC,
 EVENT_DATA_TEMPERATURE_DAY,
 EVENT_DATA_TEMPERATURE_NIGHT,
 EVENT_DATA_TEMPERATURE_CRITICAL,
 EVENT_DATA_TEMPERATURE_DELTA,*/
//EVENT_RTC_SYNKC(10, "RTC synced");

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
EVENT_SERIAL_UNKNOWN_COMMAND;


void initEvents(){
  // Use F macro to reduce requirements to memory. We can't use F macro in constructors.
  EVENT_FIRST_START_UP.init(1, F("FIRST STARTUP")), 
  EVENT_RESTART.init(2, F("RESTARTED")), 
  EVENT_MODE_DAY.init(3, F("Growbox switched to DAY mode")), 
  EVENT_MODE_NIGHT.init(4, F("Growbox switched to NIGHT mode")), 
  EVENT_LIGHT_OFF.init(5, F("LIGHT turned OFF")), 
  EVENT_LIGHT_ON.init(6, F("LIGHT turned ON")), 
  EVENT_FAN_OFF.init(7, F("FAN turned OFF")), 
  EVENT_FAN_ON_MIN.init(8, F("FAN turned ON MIN speed")), 
  EVENT_FAN_ON_MAX.init(9, F("FAN turned ON MAX speed")),
  EVENT_SERIAL_UNKNOWN_COMMAND.init(10, F("Unknown serial command"));
}
