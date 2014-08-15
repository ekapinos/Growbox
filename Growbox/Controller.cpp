#include <MemoryFree.h>
// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

#include "Controller.h"
#include "Logger.h"
#include "StorageHelper.h"


ControllerClass::ControllerClass(): 
c_freeMemoryLastCheck(0){
}

void ControllerClass::startupClock(){
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  //checkClock();
}

void ControllerClass::rebootController() {
  void(* resetFunc) (void) = 0; // Reset MC function
  resetFunc(); // call
}

void ControllerClass::checkInputPins(){

  boolean newUseSerialMonitor = (digitalRead(HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN) == HARDWARE_BUTTON_ON);

  if (newUseSerialMonitor != g_useSerialMonitor){

    if (newUseSerialMonitor){
      Serial.begin(9600);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
      } 
      g_useSerialMonitor = newUseSerialMonitor; // true
      showControllerMessage(F("Serial monitor enabled"));
    } 
    else {
      showControllerMessage(F("Serial monitor disabled"));
      Serial.flush();
      Serial.end();
      g_useSerialMonitor = newUseSerialMonitor; // false
    }
  }

  if (g_useSerialMonitor && digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON){ 
    showControllerMessage("Resetting firmware...");
    byte counter;
    for (counter = 5; counter>0; counter--){
      Serial.println(counter);
      delay(1000);
      if (digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) != HARDWARE_BUTTON_ON){
        break;
      }
    }
    if (counter == 0){
      GB_StorageHelper.resetFirmware();
      showControllerMessage("Operation finished succesfully. Remove wire from Reset pin. Device will reboot automatically");
      while(digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON) {
        delay(1000);
      }
      rebootController();
    } 
    else {
      showControllerMessage("Operation aborted");
    }
  }
}

void ControllerClass::checkFreeMemory(){

  int currentFreeMemory = freeMemory();
  if(currentFreeMemory< 2000){ 
    GB_Logger.logError(ERROR_MEMORY_LOW);
    rebootController();   
  } 
  //  else {
  //    GB_Logger.stopLogError(ERROR_MEMORY_LOW); 
  //  }

  if (c_freeMemoryLastCheck != currentFreeMemory){
    if (g_useSerialMonitor){
      showControllerMessage(F("Free memory: ["), false);  
      Serial.print(currentFreeMemory);
      Serial.println(']');
    }
    c_freeMemoryLastCheck = currentFreeMemory;
  }
}

void ControllerClass::checkClock(){

  now(); // try to refresh clock

  if (timeStatus() == timeNotSet) { 
    GB_Logger.logError(ERROR_TIMER_NOT_SET);  
  } 
  else if (timeStatus() == timeNeedsSync) { 
    GB_Logger.logError(ERROR_TIMER_NEEDS_SYNC);
  } 
  else {
    GB_Logger.stopLogError(ERROR_TIMER_NOT_SET);
    GB_Logger.stopLogError(ERROR_TIMER_NEEDS_SYNC);
  } 
}

void ControllerClass::update(){

  checkInputPins();
  checkFreeMemory();
  checkClock();

}
ControllerClass GB_Controller;








