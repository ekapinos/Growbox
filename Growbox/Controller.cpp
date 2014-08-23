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
c_freeMemoryLastCheck(0), c_isAutoCalculatedClockTimeUsed(false){
}

void ControllerClass::rebootController() {
  showControllerMessage(F("Reboot"));
  void(* resetFunc) (void) = 0; // Reset MC function
  resetFunc(); // call
}

void ControllerClass::update(){

  digitalWrite(BREEZE_PIN, !digitalRead(BREEZE_PIN));
  c_lastBreezeTimeStamp = now();

  checkInputPinsStatus();
  checkFreeMemory();

}

void ControllerClass::updateClockState(){
  // Check hardware
  if (!GB_StorageHelper.isUseRTC()){
    GB_Logger.stopLogError(ERROR_CLOCK_RTC_DISCONNECTED);
    GB_Logger.stopLogError(ERROR_CLOCK_NOT_SET);
    GB_Logger.stopLogError(ERROR_CLOCK_NEEDS_SYNC);
    return;
  }

  // Check software
  if(!isRTCPresent()){
    GB_Logger.logError(ERROR_CLOCK_RTC_DISCONNECTED); 

    GB_Logger.stopLogError(ERROR_CLOCK_NOT_SET);
    GB_Logger.stopLogError(ERROR_CLOCK_NEEDS_SYNC);
  } 
  else {
    GB_Logger.stopLogError(ERROR_CLOCK_RTC_DISCONNECTED);

    if (isClockNotSet()) { 
      GB_Logger.logError(ERROR_CLOCK_NOT_SET);  
    } 
    else if (isClockNeedsSync()) { 
      GB_Logger.logError(ERROR_CLOCK_NEEDS_SYNC);
    } 
    else {
      GB_Logger.stopLogError(ERROR_CLOCK_NOT_SET);
      GB_Logger.stopLogError(ERROR_CLOCK_NEEDS_SYNC);
    } 
  }

}

void ControllerClass::updateAutoAdjustClockTime(){
  int16_t delta = getAutoAdjustClockTimeDelta();
  if (delta == 0){
    return;
  }  
  setClockTime(now() + delta, false);
  GB_Logger.logEvent(EVENT_CLOCK_AUTO_ADJUST);
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

time_t  ControllerClass::getLastBreezeTimeStamp(){
  return c_lastBreezeTimeStamp;
}

/////////////////////////////////////////////////////////////////////
//                              CLOCK                              //
/////////////////////////////////////////////////////////////////////

boolean ControllerClass::initClock(time_t defaultTimeStamp){

  // TODO use only if GB_StorageHelper.isUseRTC() 
  setSyncProvider(RTC.get);   // the function to get the time from the RTC

  if (timeStatus() == timeNotSet){    
    setRTCandClockTimeStamp(defaultTimeStamp); 
    c_isAutoCalculatedClockTimeUsed = true;
  } 
  else {
    c_isAutoCalculatedClockTimeUsed = false;
  }

  return c_isAutoCalculatedClockTimeUsed;
}

void ControllerClass::initClock_afterLoadConfiguration(){

  if (c_isAutoCalculatedClockTimeUsed){
    GB_StorageHelper.setAutoCalculatedClockTimeUsed(true);
  }

  updateClockState();
}

boolean ControllerClass::isClockNotSet(){
  return (timeStatus() == timeNotSet);
}

boolean ControllerClass::isClockNeedsSync(){
  return (timeStatus() == timeNeedsSync);
}

// WEB command
void ControllerClass::setClockTime(time_t newTimeStamp){
  setClockTime(newTimeStamp, true);
}

//private:

void ControllerClass::setClockTime(time_t newTimeStamp, boolean checkStartupTime){
  time_t oldTimeStamp = now();
  long delta = (newTimeStamp > oldTimeStamp) ? (newTimeStamp - oldTimeStamp) : -((long)(oldTimeStamp - newTimeStamp));

  setRTCandClockTimeStamp(newTimeStamp);

  if (GB_StorageHelper.isAutoCalculatedClockTimeUsed()){

    GB_StorageHelper.setAutoCalculatedClockTimeUsed(false);

  }
  
  c_lastBreezeTimeStamp += delta;
  GB_Watering.adjustLastWatringTimeOnClockSet(delta); 
  
  // If we started with clean RTC we should update time stamps
  if (!checkStartupTime){
    return;
  } 
  if (GB_StorageHelper.getStartupTimeStamp() == 0){
    GB_StorageHelper.adjustStartupTimeStamp(delta);  
  }
  if (GB_StorageHelper.getFirstStartupTimeStamp() == 0){
    GB_StorageHelper.adjustFirstStartupTimeStamp(delta);
  }  
}
//public: 

// WEB command
void ControllerClass::setAutoAdjustClockTimeDelta(int16_t delta){
  GB_StorageHelper.setAutoAdjustClockTimeDelta(delta);
}

int16_t ControllerClass::getAutoAdjustClockTimeDelta(){
  return GB_StorageHelper.getAutoAdjustClockTimeDelta();
}

// WEB command
void ControllerClass::setUseRTC(boolean flag){
  GB_StorageHelper.setUseRTC(flag); 

  setSyncProvider(RTC.get);   // Force resync Clock with RTC 
  updateClockState();
}

boolean ControllerClass::isUseRTC(){
  return GB_StorageHelper.isUseRTC();
}

boolean ControllerClass::isRTCPresent(){
  RTC.get(); // update status
  return RTC.chipPresent();
}

// private:

void ControllerClass::setRTCandClockTimeStamp(time_t newTimeStamp){

  RTC.set(newTimeStamp);
  setTime(newTimeStamp);

  if (g_useSerialMonitor) {
    showControllerMessage(F("Set new Clock time ["), false);
    Serial.print(StringUtils::timeStampToString(newTimeStamp));
    Serial.println(F("] "));
  }
}

ControllerClass GB_Controller;






















