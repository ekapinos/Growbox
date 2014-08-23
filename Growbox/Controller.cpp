#include <MemoryFree.h>

#include <Time.h>
// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

#include "Controller.h"
#include "Logger.h"
#include "StorageHelper.h"
#include "Watering.h"


ControllerClass::ControllerClass(): 
c_freeMemoryLastCheck(0), c_isAutoCalculatedTimeStampUsed(false){
}

void ControllerClass::rebootController() {
  showControllerMessage(F("Reboot"));
  void(* resetFunc) (void) = 0; // Reset MC function
  resetFunc(); // call
}

void ControllerClass::checkInputPinsStatus(boolean checkFirmwareReset){

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

  if (checkFirmwareReset && g_useSerialMonitor && digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON){ 
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

void ControllerClass::update(){
  checkInputPinsStatus();
  checkFreeMemory();
}

void ControllerClass::initClock(time_t defaultTimeStamp){

  setSyncProvider(RTC.get);   // the function to get the time from the RTC

  if (timeStatus() == timeNotSet){    
    setHarwareAndSoftwareClockTimeStamp(defaultTimeStamp); 
    c_isAutoCalculatedTimeStampUsed = true;
  }
}

void ControllerClass::initClock_afterLoadConfiguration(){
//  if (c_isAutoCalculatedTimeStampUsed){
    // After init_loadConfiguration, do not clear flag
    GB_StorageHelper.setClockTimeStampAutoCalculated(c_isAutoCalculatedTimeStampUsed);
//  }
}

void ControllerClass::setClockTime(time_t newTimeStamp){
  
  time_t oldTimeStamp = now();
  long delta = (newTimeStamp > oldTimeStamp) ? (newTimeStamp - oldTimeStamp) : -((long)(oldTimeStamp - newTimeStamp));
  
  setHarwareAndSoftwareClockTimeStamp(newTimeStamp);
  
  if (GB_StorageHelper.isClockTimeStampAutoCalculated()){
    
    GB_StorageHelper.setClockTimeStampAutoCalculated(false);
    
//    GB_StorageHelper.adjustLastStartupTimeStamp(delta);  
//    if (GB_StorageHelper.getFirstStartupTimeStamp() == 0){
//      GB_StorageHelper.adjustFirstStartupTimeStamp(delta);
//    }  
  }
  
  // Update watering time
  GB_Watering.adjustWatringTimeOnClockSet(delta); 
}

// private:

void ControllerClass::setHarwareAndSoftwareClockTimeStamp(time_t newTimeStamp){

  RTC.set(newTimeStamp);
  setTime(newTimeStamp);

  if (g_useSerialMonitor) {
    showControllerMessage(F("Set new Clock time ["), false);
    Serial.print(StringUtils::timeStampToString(newTimeStamp));
    Serial.println(F("] "));
  }
}

// public:

void ControllerClass::updateClockState(){
  
  if (!GB_StorageHelper.isUseRTC()){
    return;
  }
  
  now(); // try to resync clock with hardware
  
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

boolean ControllerClass::isRTCPresent(){
  RTC.get(); // update status
  return RTC.chipPresent();
}

//boolean ControllerClass::isAutoCalculatedTimeUsed(){
//  return c_clockIsAutoCalculatedTimeUsed;
//}


ControllerClass GB_Controller;










