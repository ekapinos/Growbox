#include "Error.h"

Error* Error::lastAddedItem = 0;

Error 
ERROR_TIMER_NOT_SET,
ERROR_TIMER_NEEDS_SYNC,
ERROR_TERMOMETER_DISCONNECTED,
ERROR_TERMOMETER_ZERO_VALUE,
ERROR_TERMOMETER_CRITICAL_VALUE,
ERROR_MEMORY_LOW;


void initErrors(){
  // Use F macro to reduce requirements to memory. We can't use F macro in constructors.
  ERROR_TIMER_NOT_SET.init(B00, 2, F("Error: Timer not set"));
  ERROR_TIMER_NEEDS_SYNC.init(B001, 3, F("Error: Timer needs sync"));
  ERROR_TERMOMETER_DISCONNECTED.init(B01, 2, F("Error: Termometer disconnected"));
  ERROR_TERMOMETER_ZERO_VALUE.init(B010, 3, F("Error: Termometer returned ZERO value"));
  ERROR_TERMOMETER_CRITICAL_VALUE.init(B000, 3, F("Error: Termometer returned CRITICAL value"));
  ERROR_MEMORY_LOW.init(B111, 3, F("Error: Memory remained less 200 bytes"));
}
