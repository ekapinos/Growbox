#include <MemoryFree.h>

#include "Controller.h"
#include "Logger.h"

void ControllerClass::rebootController() {
  void(* resetFunc) (void) = 0; // Reset MC function
  resetFunc(); // call
}

void ControllerClass::checkFreeMemory(){
  if(freeMemory() < 200){ 
    GB_Logger.logError(ERROR_MEMORY_LOW);   
  } 
  else {
    GB_Logger.stopLogError(ERROR_MEMORY_LOW); 
  }
}

void ControllerClass::updateSerialMonitorStatus(){

  boolean oldUseSerialMonitor  = g_useSerialMonitor;

  g_useSerialMonitor = (digitalRead(USE_SERIAL_MONOTOR_PIN) == SERIAL_MONITOR_ON);

  if (oldUseSerialMonitor != g_useSerialMonitor){

    if (g_useSerialMonitor){
      Serial.begin(9600);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
      } 
      Serial.print(F("Serial monitor: "));
      Serial.println(FS(S_Enabled)); // shows when useSerialMonitor=false
    } 
    else {
      Serial.println(FS(S_Disabled));
      Serial.end();
    }
  } 
}

ControllerClass GB_Controller;

