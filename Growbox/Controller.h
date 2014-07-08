#ifndef GB_Controller_h
#define GB_Controller_h

#include "Global.h"
#include "Logger.h"

class GB_Controller{

public:

  static void rebootController() {
    void(* resetFunc) (void) = 0; // Reset MC function
    resetFunc(); // call
  }

  // discover-memory-overflow-errors-in-the-arduino-c-code
  static void checkFreeMemory(){
    if(freeMemory() < 200){ 
      GB_Logger::logError(ERROR_MEMORY_LOW);   
    } 
    else {
      GB_Logger::stopLogError(ERROR_MEMORY_LOW); 
    }
  }
};

#endif

