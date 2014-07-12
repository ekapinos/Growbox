#include <MemoryFree.h>

#include "Controller.h"
#include "Logger.h"

void ControllerClass::rebootController() {
  void(* resetFunc) (void) = 0; // Reset MC function
  resetFunc(); // call
}

void ControllerClass::checkFreeMemory(){
  if(freeMemory() < 200){ 
    GB_LOGGER.logError(ERROR_MEMORY_LOW);   
  } 
  else {
    GB_LOGGER.stopLogError(ERROR_MEMORY_LOW); 
  }
}

ControllerClass GB_CONTROLLER;
