#include <MemoryFree.h>

#include <avr/wdt.h>
#include <Time.h>
// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

#include "Controller.h"
#include "Logger.h"
#include "StorageHelper.h"
#include "Watering.h"

ControllerClass::ControllerClass() :
    c_lastFreeMemory(0),
    c_isAutoCalculatedClockTimeUsed(false),
    c_lastBreezeTimeStamp(0),
    c_isDayInGrowbox(-1),
    c_fan_cycleCounter(0),
    c_fan_isOn(false),
    c_fan_speed(RELAY_OFF),
    c_fan_numerator(0),
    c_fan_denominator(0) {
  // We set c_isDayInGrowbox == -1 to force log on startup
}

void ControllerClass::rebootController() {
  showControllerMessage(F("Reboot"));
  void (*resetFunc)(void) = 0; // initialize Software Reset function
  resetFunc(); // call zero pointer
}

void ControllerClass::update() {

  updateBreeze();

  checkInputPinsStatus();
  checkFreeMemory();

  updateFan();
}

void ControllerClass::updateBreeze() {

  digitalWrite(BREEZE_PIN, !digitalRead(BREEZE_PIN));

  if (CONTROLLER_USE_WATCH_DOG_TIMER) {
    wdt_reset();
  }

  c_lastBreezeTimeStamp = now();

}
void ControllerClass::updateClockState() {
  // Check hardware
  if (!GB_StorageHelper.isUseRTC()) {
    GB_Logger.stopLogError(ERROR_CLOCK_RTC_DISCONNECTED);
    GB_Logger.stopLogError(ERROR_CLOCK_NOT_SET);
    GB_Logger.stopLogError(ERROR_CLOCK_NEEDS_SYNC);
    return;
  }

  // Check software
  if (!isRTCPresent()) {
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

void ControllerClass::updateAutoAdjustClockTime() {
  int16_t delta = getAutoAdjustClockTimeDelta();
  if (delta == 0) {
    return;
  }
  setClockTime(now() + delta, false);
  GB_Logger.logEvent(EVENT_CLOCK_AUTO_ADJUST);
}

void ControllerClass::checkInputPinsStatus(boolean checkFirmwareReset) {

  boolean newUseSerialMonitor = (digitalRead(HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN) == HARDWARE_BUTTON_ON);

  if (newUseSerialMonitor != g_useSerialMonitor) {

    if (newUseSerialMonitor) {
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

  if (checkFirmwareReset && g_useSerialMonitor && digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON) {
    showControllerMessage("Resetting firmware...");
    byte counter;
    for (counter = 5; counter > 0; counter--) {
      Serial.println(counter);
      delay(1000);
      if (digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) != HARDWARE_BUTTON_ON) {
        break;
      }
    }
    if (counter == 0) {
      GB_StorageHelper.resetFirmware();
      showControllerMessage("Operation finished successfully. Remove wire from Reset pin. Device will reboot automatically");
      while (digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON) {
        delay(1000);
      }
      rebootController();
    }
    else {
      showControllerMessage("Operation aborted");
    }
  }
}

void ControllerClass::checkFreeMemory() {

  int currentFreeMemory = freeMemory();
  if (currentFreeMemory < 2000) {
    GB_Logger.logError(ERROR_MEMORY_LOW);
    rebootController();
  }
  // no sense if reboot
  //  else {
  //    GB_Logger.stopLogError(ERROR_MEMORY_LOW); 
  //  }

  if (c_lastFreeMemory != currentFreeMemory) {
    if (g_useSerialMonitor) {
      showControllerMessage(F("Free memory: ["), false);
      Serial.print(currentFreeMemory);
      Serial.println(']');
    }
    c_lastFreeMemory = currentFreeMemory;
  }
}

boolean ControllerClass::isBreezeFatalError() {
  return ((now() - c_lastBreezeTimeStamp) > 10UL); // No breeze more than 10 seconds
}

/////////////////////////////////////////////////////////////////////
//                              CLOCK                              //
/////////////////////////////////////////////////////////////////////

boolean ControllerClass::initClock(time_t lastStoredTimeStamp) {

  // TODO use only if GB_StorageHelper.isUseRTC() 
  setSyncProvider(RTC.get);   // the function to get the time from the RTC

  if (isClockNotSet() || lastStoredTimeStamp > now()) {
    // second expression can appear, if something wrong with RTC
    if (lastStoredTimeStamp != 0) {
      lastStoredTimeStamp += SECS_PER_MIN; // if not first start, we set +minute as default time
    }
    setRTCandClockTimeStamp(lastStoredTimeStamp);
    c_isAutoCalculatedClockTimeUsed = true;
  }
  else {
    c_isAutoCalculatedClockTimeUsed = false;
  }

  return c_isAutoCalculatedClockTimeUsed;
}

void ControllerClass::initClock_afterLoadConfiguration() {

  if (c_isAutoCalculatedClockTimeUsed) {
    GB_StorageHelper.setAutoCalculatedClockTimeUsed(true);
  }
  c_isAutoCalculatedClockTimeUsed = false; // we will not use this variable. Call GB_StorageHelper.getAutoCalculatedClockTimeUsed(); instead
  updateClockState();
}

boolean ControllerClass::isClockNotSet() {
  return (timeStatus() == timeNotSet);
}

boolean ControllerClass::isClockNeedsSync() {
  return (timeStatus() == timeNeedsSync);
}

// WEB command
void ControllerClass::setClockTime(time_t newTimeStamp) {
  setClockTime(newTimeStamp, true);
}

//private:

void ControllerClass::setClockTime(time_t newTimeStamp, boolean checkStartupTime) {
  time_t oldTimeStamp = now();
  long delta =
      (newTimeStamp > oldTimeStamp) ? (newTimeStamp - oldTimeStamp) : -((long)(oldTimeStamp - newTimeStamp));

  setRTCandClockTimeStamp(newTimeStamp);

  if (GB_StorageHelper.isAutoCalculatedClockTimeUsed()) {

    GB_StorageHelper.setAutoCalculatedClockTimeUsed(false);

  }

  c_lastBreezeTimeStamp += delta;
  GB_Watering.adjustLastWatringTimeOnClockSet(delta);
  // TODO what about wi-fi last active time stamp?

  // Time Alarms
  for (int alarmId = 0; alarmId < dtNBR_ALARMS; alarmId++){
    if (Alarm.isAllocated(alarmId)){
      Alarm.enable(alarmId); // see comment inside cpp: trigger is updated whenever  this is called, even if already enabled
    }
  }

  // If we started with clean RTC we should update time stamps
  if (!checkStartupTime) {
    return;
  }
  time_t firstStartupTimeStamp = GB_StorageHelper.getFirstStartupTimeStamp();
  if (firstStartupTimeStamp == 0 || firstStartupTimeStamp > newTimeStamp) {
    GB_StorageHelper.setFirstStartupTimeStamp(newTimeStamp);
  }
  time_t startupTimeStamp = GB_StorageHelper.getStartupTimeStamp();
  if (startupTimeStamp == 0 || startupTimeStamp > newTimeStamp) {
    GB_StorageHelper.setStartupTimeStamp(newTimeStamp);
  }
}
//public: 

// WEB command
void ControllerClass::setAutoAdjustClockTimeDelta(int16_t delta) {
  GB_StorageHelper.setAutoAdjustClockTimeDelta(delta);
}

int16_t ControllerClass::getAutoAdjustClockTimeDelta() {
  return GB_StorageHelper.getAutoAdjustClockTimeDelta();
}

// WEB command
void ControllerClass::setUseRTC(boolean flag) {
  GB_StorageHelper.setUseRTC(flag);

  setSyncProvider(RTC.get);   // Force resync Clock with RTC 
  updateClockState();
}

boolean ControllerClass::isUseRTC() {
  return GB_StorageHelper.isUseRTC();
}

boolean ControllerClass::isRTCPresent() {
  RTC.get(); // update status
  return RTC.chipPresent();
}

// private:

void ControllerClass::setRTCandClockTimeStamp(time_t newTimeStamp) {

  RTC.set(newTimeStamp);
  setTime(newTimeStamp);

  if (g_useSerialMonitor) {
    showControllerMessage(F("Set new Clock time ["), false);
    Serial.print(StringUtils::timeStampToString(newTimeStamp));
    Serial.println(F("] "));
  }
}

/////////////////////////////////////////////////////////////////////
//                              DEVICES                            //
/////////////////////////////////////////////////////////////////////

// public:

// Growbox

boolean ControllerClass::isDayInGrowbox(boolean update){

  if (!update) {
    return c_isDayInGrowbox;
  }
  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  word currentTime = hour() * 60 + minute();

  boolean isDayInGrowbox = false;
  if (upTime == downTime) {
    isDayInGrowbox = true; // Always day
  }
  else if (upTime < downTime) {
    if (upTime < currentTime && currentTime < downTime) {
      isDayInGrowbox = true;
    }
  }
  else { // upTime > downTime
    if (upTime < currentTime || currentTime < downTime) {
      isDayInGrowbox = true;
    }
  }
  // Log on change
  if (c_isDayInGrowbox != isDayInGrowbox) {
    if (isDayInGrowbox) {
      GB_Logger.logEvent(EVENT_MODE_DAY);
    } else {
      GB_Logger.logEvent(EVENT_MODE_NIGHT);
    }
  }
  c_isDayInGrowbox = isDayInGrowbox;
  return c_isDayInGrowbox;
}

// Light

void ControllerClass::setUseLight(boolean flag) {
  if (isUseLight() == flag) {
    return;
  }
  turnOffLight();
  GB_StorageHelper.setUseLight(flag);
  GB_Logger.logEvent(flag ? EVENT_LIGHT_ENABLED : EVENT_LIGHT_DISABLED);
}

boolean ControllerClass::isUseLight() {
  return GB_StorageHelper.isUseLight();
}

void ControllerClass::turnOnLight() {
  if (!isUseLight()) {
    return;
  }
  if (digitalRead(LIGHT_PIN) == RELAY_ON) {
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_ON);
  GB_Logger.logEvent(EVENT_LIGHT_ON);
}

void ControllerClass::turnOffLight() {
  if (!isUseLight()) {
    return;
  }
  if (digitalRead(LIGHT_PIN) == RELAY_OFF) {
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_OFF);
  GB_Logger.logEvent(EVENT_LIGHT_OFF);
}

boolean ControllerClass::isLightTurnedOn() {
  return (digitalRead(LIGHT_PIN) == RELAY_ON);
}

// Fan

void ControllerClass::setUseFan(boolean flag) {
  if (isUseFan() == flag) {
    return;
  }
  turnOffFan();
  GB_StorageHelper.setUseFan(flag);
  GB_Logger.logEvent(flag ? EVENT_FAN_ENABLED : EVENT_FAN_DISABLED);
}

boolean ControllerClass::isUseFan() {
  return GB_StorageHelper.isUseFan();
}

boolean ControllerClass::isFanTurnedOn() {
  return c_fan_isOn;
}

boolean ControllerClass::isFanHardwareTurnedOn() {
  return (digitalRead(FAN_PIN) == RELAY_ON);
}
boolean ControllerClass::isFanUseRatio(){
  return (c_fan_numerator != 0 && c_fan_denominator != 0);
}
void ControllerClass::getNumeratorDenominatorByIndex (byte index, byte& numerator, byte& denominator) {
  if (index == 0){ // full time
    numerator = 0; denominator = 0;
  } else if (index == 1){ // 5/10
    numerator = 1; denominator = 2;
  }
  else if (index == 2){ // 5/15
    numerator = 1; denominator = 3;
  }
  else if (index == 3){ // 5/20
    numerator = 1; denominator = 4;
  }
  else if (index == 4){ // 5/30
    numerator = 1; denominator = 6;
  }
  else if (index == 5){ // 5/60
    numerator = 1; denominator = 12;
  }
  else if (index == 6){ // 10/15
    numerator = 2; denominator = 3;
  }
  else if (index == 7){ // 10/20
    numerator = 2; denominator = 4;
  }
  else if (index == 8){ // 10/30
    numerator = 2; denominator = 6;
  }
  else if (index == 9){ // 10/60
    numerator = 2; denominator = 12;
  }
  else if (index == 10){ // 15/20
    numerator = 3; denominator = 4;
  }
  else if (index == 11){ // 15/30
    numerator = 3; denominator = 6;
  }
  else if (index == 12){ // 15/60
    numerator = 3; denominator = 12;
  }
  else if (index == 13){ // 20/30
    numerator = 4; denominator =6 ;
  }
  else if (index == 14){ // 20/60
    numerator = 4; denominator = 12;
  }
  else if (index == 15){ // 30/60
    numerator = 6; denominator = 12;
  } else {
    numerator = 0; denominator = 0; // stop marker
  }
}

byte ControllerClass::numeratorDenominatorCombinationsCount(){
  byte index = 1;
  byte numerator, denominator;
  getNumeratorDenominatorByIndex(index, numerator, denominator);
  while (numerator != 0 && denominator!= 0){ // look for stop marker
    index++;
    getNumeratorDenominatorByIndex(index, numerator, denominator);
  }
  return index;
}

byte ControllerClass::findNuneratorDenominatorCombinationIndex(byte numerator, byte denominator){
  byte l_numerator, l_denominator;
  for (byte index = 0; index < numeratorDenominatorCombinationsCount(); index++){
    getNumeratorDenominatorByIndex(index, l_numerator, l_denominator);
    if (l_numerator == numerator  && l_denominator == denominator){
      return index;
    }
  }
  return 0; // full time
}

byte ControllerClass::packFanSpeedValue(boolean isOn, byte speed, byte numerator, byte denominator) {
  if (isOn){
    return B10000000 |
        (speed == FAN_SPEED_HIGH ? B01000000 : 0) |
        (B00111111 & findNuneratorDenominatorCombinationIndex(numerator, denominator));
  } else {
    return 0;
  }
}

void ControllerClass::unpackFanSpeedValue(byte fanSpeedValue, boolean& isOn, byte& speed, byte& numerator, byte& denominator) {
  isOn = (fanSpeedValue & B10000000) > 0;
  speed = (fanSpeedValue & B01000000) > 0 ? FAN_SPEED_HIGH : FAN_SPEED_LOW;

  byte numeratorDenominatorIndex = (fanSpeedValue & B00111111);
  getNumeratorDenominatorByIndex(numeratorDenominatorIndex, numerator, denominator);
}

void ControllerClass::turnOnOffFanBySpeedValue(byte fanSpeedValue){

  boolean isOn; byte speed, numerator, denominator;
  unpackFanSpeedValue(fanSpeedValue, isOn, speed, numerator, denominator);

  if (isOn){
    turnOnFan(speed, numerator, denominator);
  }
  else {
    turnOffFan();
  }
}

byte ControllerClass::getCurrentFanSpeedValue() {
  return packFanSpeedValue(c_fan_isOn, c_fan_speed, c_fan_numerator, c_fan_denominator);
}

void ControllerClass::turnOnFan(byte speed, byte numerator, byte denominator) {
  if (!isUseFan()) {
    return;
  }
  if (c_fan_isOn && c_fan_speed == speed && numerator == c_fan_numerator && denominator == c_fan_denominator) {
    return;
  }

  if (numerator > B1111 || denominator > B1111 || numerator > denominator ){ // Error argument
    numerator = 0;
    denominator = 0;
  }
  c_fan_isOn = true;
  c_fan_speed = speed;
  c_fan_numerator = numerator;
  c_fan_denominator = denominator;

  if (denominator != 0) {
    c_fan_cycleCounter = denominator-1; // Zero for speed without ratio
  } else {
    c_fan_cycleCounter = 0;
  }
  if(g_useSerialMonitor){
    showControllerMessage(F("turnOnFan, counter="), false);
    Serial.println(c_fan_cycleCounter);
  }

  byte logData = (c_fan_numerator << 4) | denominator;
  if (speed == FAN_SPEED_LOW) {
    GB_Logger.logEvent(EVENT_FAN_ON_LOW, logData); // TODO save as fanSpeedValue
  }
  else {
    GB_Logger.logEvent(EVENT_FAN_ON_HIGH, logData);
  }

  updateFan();
}

void ControllerClass::turnOffFan() {

  c_fan_cycleCounter = 0;
  if(g_useSerialMonitor){
    showControllerMessage(F("turnOffFan, counter="), false);
    Serial.println(c_fan_cycleCounter);
  }

  if (!isUseFan()) {
    return;
  }
  if (!c_fan_isOn) {
    return;
  }
  c_fan_isOn = false;

  GB_Logger.logEvent(EVENT_FAN_OFF);

  updateFan();
}

void ControllerClass::updateFan() {
  boolean isFanOnNow = c_fan_isOn;
  if (isFanTurnedOn() && isFanUseRatio()){
    isFanOnNow = (c_fan_denominator - c_fan_cycleCounter) <= c_fan_numerator;
  }

  if (isFanHardwareTurnedOn() != isFanOnNow){
    showControllerMessage(isFanOnNow ? F("hardware fan on") : F("hardware fan off"));
  }

  if (isFanOnNow){
    digitalWrite(FAN_SPEED_PIN, c_fan_speed);
    digitalWrite(FAN_PIN, RELAY_ON);
  }
  else {
    digitalWrite(FAN_PIN, RELAY_OFF);
    digitalWrite(FAN_SPEED_PIN, RELAY_OFF);
  }
}

boolean ControllerClass::isCurrentFanCycleFinished(){
  return c_fan_cycleCounter == 0;
}

void ControllerClass::setNextFanCycleStep(){
  if (c_fan_cycleCounter > 0){
    c_fan_cycleCounter--;
  } else if (c_fan_denominator != 0) {
    c_fan_cycleCounter = c_fan_denominator-1;
  } else {
    c_fan_cycleCounter = 0;
  }
  if(g_useSerialMonitor){
    showControllerMessage(F("setNextFanCycleStep, counter="), false);
    Serial.println(c_fan_cycleCounter);
  }
}

byte ControllerClass::getCurrentFanCycleStep(){
  return c_fan_denominator - c_fan_cycleCounter;
}


// Heater

void ControllerClass::setUseHeater(boolean flag) {
  if (isUseHeater() == flag) {
    return;
  }
  turnOffHeater();
  GB_StorageHelper.setUseHeater(flag);
  GB_Logger.logEvent(flag ? EVENT_HEATER_ENABLED : EVENT_HEATER_DISABLED);
}

boolean ControllerClass::isUseHeater() {
  return GB_StorageHelper.isUseHeater();
}

void ControllerClass::turnOnHeater() {
  if (!isUseHeater()) {
    return;
  }
  if (digitalRead(HEATER_PIN) == RELAY_ON) {
    return;
  }
  digitalWrite(HEATER_PIN, RELAY_ON);
  GB_Logger.logEvent(EVENT_HEATER_ON);
}

void ControllerClass::turnOffHeater() {
  if (!isUseHeater()) {
    return;
  }
  if (digitalRead(HEATER_PIN) == RELAY_OFF) {
    return;
  }
  digitalWrite(HEATER_PIN, RELAY_OFF);
  GB_Logger.logEvent(EVENT_HEATER_OFF);
}

boolean ControllerClass::isHeaterTurnedOn() {
  return (digitalRead(HEATER_PIN) == RELAY_ON);
}

ControllerClass GB_Controller;

