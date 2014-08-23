#include "Watering.h"

#include "StorageHelper.h"
#include "Logger.h"

time_t WateringClass::c_turnOnWetSensorsTimeStamp = 0;
byte WateringClass::c_lastWetSensorValue[MAX_WATERING_SYSTEMS_COUNT];

TimeAlarmsClass WateringClass::c_PumpOnAlarm;
TimeAlarmsClass WateringClass::c_PumpOffAlarm;

AlarmID_t WateringClass::c_PumpOnAlarmIDArray[MAX_WATERING_SYSTEMS_COUNT];
AlarmID_t WateringClass::c_PumpOffAlarmIDArray[MAX_WATERING_SYSTEMS_COUNT];

void WateringClass::init(time_t preUpdateWetSatusTimeStamp){

  c_turnOnWetSensorsTimeStamp = preUpdateWetSatusTimeStamp;
  boolean isSensorsTurnedOn = false;
  
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

    if (wsp.boolPreferencies.isWetSensorConnected){
      // We will use turned on state after in main file
      c_lastWetSensorValue[wsIndex] = WATERING_UNSTABLE_VALUE; // Not read yet  
      isSensorsTurnedOn = true;
    } 
    else {
      // Turn OFF unused Wet sensor pins
      c_lastWetSensorValue[wsIndex] = WATERING_DISABLE_VALUE; 
      digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], LOW);
    }

    c_PumpOnAlarmIDArray[wsIndex] = dtINVALID_ALARM_ID;
    c_PumpOffAlarmIDArray[wsIndex] = dtINVALID_ALARM_ID;
  }
  
  if (!isSensorsTurnedOn){
    c_turnOnWetSensorsTimeStamp = 0; // clear to call turnOffWetSensorsAndUpdateWetStatus correctly if no sensors present
  }
  
}

void WateringClass::updateAlarms(){
  c_PumpOnAlarm.delay(0);
  c_PumpOffAlarm.delay(0);
}

void WateringClass::adjustWatringTimeOnClockSet(long delta) {
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (wsp.lastWateringTimeStamp == 0){
      continue;
    }
    wsp.lastWateringTimeStamp += delta;
    GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);
  }
  updateWateringSchedule();
}

/////////////////////////////////////////////////////////////////////
//                           WET SENSORS                           //
/////////////////////////////////////////////////////////////////////

boolean WateringClass::preUpdateWetSatus(){

  if (c_turnOnWetSensorsTimeStamp != 0){
    return true; // already turned on// TODO check it when preferences changing
  }

  c_turnOnWetSensorsTimeStamp = now();
  boolean isSensorsTurnedOn = false;

  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (wsp.boolPreferencies.isWetSensorConnected){
      showWateringMessage(wsIndex, F("Wet sensor ON"));
      digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], HIGH);
      isSensorsTurnedOn = true;
    }
    else {
      // Used on startup
      if (digitalRead(WATERING_WET_SENSOR_POWER_PINS[wsIndex]) != LOW){
          digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], LOW);
          showWateringMessage(wsIndex, F("Wet sensor OFF"));
      }
     
      if (c_lastWetSensorValue[wsIndex] != WATERING_DISABLE_VALUE){
        c_lastWetSensorValue[wsIndex] = WATERING_DISABLE_VALUE;

        GB_Logger.logWateringEvent(wsIndex, WATERING_EVENT_WET_SENSOR_DISABLED, WATERING_DISABLE_VALUE);  

      }
    }
  }

  if (!isSensorsTurnedOn){
    c_turnOnWetSensorsTimeStamp = 0; // clear to call turnOffWetSensorsAndUpdateWetStatus correctly if no sensors present
  }
  
  return isSensorsTurnedOn;
}

void WateringClass::updateWetSatus(){

  if (c_turnOnWetSensorsTimeStamp == 0){
    if (!preUpdateWetSatus()){
      return; // No one sensor were turned on
    }
  }  

  long remanedDelay = c_turnOnWetSensorsTimeStamp - now() + WATERING_SYSTEM_TURN_ON_DELAY;
  if (remanedDelay > 0){
    if(g_useSerialMonitor){
      showWateringMessage(F("Waiting Wet sensors for "), false);
      Serial.print(remanedDelay);
      Serial.println(F(" sec"));
    }
    delay((unsigned long) remanedDelay * 1000);
  }

  c_turnOnWetSensorsTimeStamp = 0; // clear 

  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){

    // Maybe Wet sensor was already updated (between scheduled call preUpdateWetSatus and updateWetStatus was WEB call with force update) 
    if (digitalRead(WATERING_WET_SENSOR_POWER_PINS[wsIndex]) == LOW){
      continue;
    }

    byte wetValue = readWetValue(wsIndex);
    digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], LOW);

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

    WateringEvent* oldState = valueToState(wsp, c_lastWetSensorValue[wsIndex]);
    WateringEvent* newState = valueToState(wsp, wetValue);

    //showWateringMessage(wsIndex, newState->shortDescription);
    showWateringMessage(wsIndex, F("Wet sensor OFF"));

    if (oldState != newState){
      GB_Logger.logWateringEvent(wsIndex, *newState, wetValue);  
    }

    c_lastWetSensorValue[wsIndex] = wetValue;

  }
}


/////////////////////////////////////////////////////////////////////
//                           WATER PUMPS                           //
/////////////////////////////////////////////////////////////////////

// public:

void WateringClass::updateWateringSchedule(){  
  // If Growbox miss Watering during Power Off, start it immediately
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    scheduleNextWateringTime(wsIndex);
  }
}

time_t WateringClass::getLastWateringTimeStampByIndex(byte wsIndex){
  if (wsIndex >= MAX_WATERING_SYSTEMS_COUNT){
    return 0;
  }
  BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
  return wsp.lastWateringTimeStamp;
}

time_t WateringClass::getNextWateringTimeStampByIndex(byte wsIndex){

  //  Serial.println();
  if (wsIndex >= MAX_WATERING_SYSTEMS_COUNT){
    showWateringMessage(F("getNextWateringTimeStampByIndex: wsIndex >= MAX_WATERING_SYSTEMS_COUNT"));
    return 0;
  }
  if (c_PumpOnAlarmIDArray[wsIndex] == dtINVALID_ALARM_ID){
    showWateringMessage(F("getNextWateringTimeStampByIndex: c_PumpOnAlarmIDArray[wsIndex] == dtINVALID_ALARM_ID"));
    return 0;
  }

  //  showWateringMessage(F("getNextWateringTimeStampByIndex: c_PumpOnAlarmIDArray[wsIndex]"), false);
  //  Serial.println(c_PumpOnAlarmIDArray[wsIndex]);
  //  showWateringMessage(F("getNextWateringTimeStampByIndex: c_PumpOnAlarm.read(c_PumpOnAlarmIDArray[wsIndex])"), false);
  //  Serial.println( c_PumpOnAlarm.read(c_PumpOnAlarmIDArray[wsIndex]));
  //  Serial.println(StringUtils::timeStampToString( c_PumpOnAlarm.read(c_PumpOnAlarmIDArray[wsIndex])));


  return c_PumpOnAlarm.read(c_PumpOnAlarmIDArray[wsIndex]);
}

void WateringClass::turnOnWaterPumpManual(byte wsIndex){
  turnOnWaterPumpByIndex(wsIndex, false);
}

// private:

void WateringClass::scheduleNextWateringTime(byte wsIndex){

  time_t currentTimeStamp = now();

  //  Serial.println("a");

  BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

  if (c_PumpOnAlarmIDArray[wsIndex] != dtINVALID_ALARM_ID){
    c_PumpOnAlarm.free(c_PumpOnAlarmIDArray[wsIndex]);
    c_PumpOnAlarmIDArray[wsIndex] = dtINVALID_ALARM_ID;
  }

  if (!wsp.boolPreferencies.isWaterPumpConnected){
    return;
  }

  time_t nextNormalScheduleTimeStamp = currentTimeStamp - elapsedSecsToday(currentTimeStamp) + wsp.startWateringAt * SECS_PER_MIN;

  //  Serial.print("currentTimeStamp: ");
  //  Serial.println(StringUtils::timeStampToString(currentTimeStamp));  
  //  Serial.print("nextNormalScheduleTimeStamp: ");
  // Serial.println(StringUtils::timeStampToString(nextNormalScheduleTimeStamp));

  if (nextNormalScheduleTimeStamp < currentTimeStamp){
    nextNormalScheduleTimeStamp += SECS_PER_DAY;
    //     Serial.println("b");
  }

  //  Serial.print("nextNormalScheduleTimeStamp: ");
  //  Serial.println(StringUtils::timeStampToString(nextNormalScheduleTimeStamp));

  if (wsp.lastWateringTimeStamp == 0){
    // All OK, schedule watering without delta
    c_PumpOnAlarmIDArray[wsIndex] = c_PumpOnAlarm.triggerOnce(nextNormalScheduleTimeStamp, turnOnWaterPumpOnSchedule);
    //    Serial.println("c");
    //     Serial.println(c_PumpOnAlarmIDArray[wsIndex]);
    return;
  }

  // Check for force Watering
  if (currentTimeStamp - SECS_PER_DAY - WATERING_ERROR_DELTA > wsp.lastWateringTimeStamp){

    // Watering wasn't during last 24 hours, we need start watering right now
    if (g_useSerialMonitor){
      time_t missedTime = currentTimeStamp - SECS_PER_DAY - wsp.lastWateringTimeStamp;
      missedTime /= 60; // in minutes

      showWateringMessage(wsIndex, F("Force Watering now. Last watering was ["));
      Serial.print(StringUtils::timeStampToString(wsp.lastWateringTimeStamp));
      Serial.print(F("] , missed time ["));
      Serial.print(missedTime/60);
      Serial.print(F("h "));
      Serial.print(missedTime%60);
      Serial.print(F("m]"));
      Serial.println();
    }
    turnOnWaterPumpByIndex(wsIndex, true);

    //    Serial.println("d");

    return;
  }

  // Watering was during last 24 hours

  // Find nearest normal schedule TimeStamp
  time_t calculatedNextTimeStamp = wsp.lastWateringTimeStamp + SECS_PER_DAY;

  //  Serial.print("calculatedNextTimeStamp: ");
  //  Serial.println(StringUtils::timeStampToString(calculatedNextTimeStamp));

  time_t nextNextNormalScheduleTimeStamp = nextNormalScheduleTimeStamp + SECS_PER_DAY;

  //  Serial.print("nextNextNormalScheduleTimeStamp: ");
  //  Serial.println(StringUtils::timeStampToString(nextNextNormalScheduleTimeStamp));


  long nextDeltaAbs = (nextNormalScheduleTimeStamp > calculatedNextTimeStamp) ? (nextNormalScheduleTimeStamp - calculatedNextTimeStamp) : (calculatedNextTimeStamp - nextNormalScheduleTimeStamp);
  long nextNextDeltaAbs = (nextNextNormalScheduleTimeStamp > calculatedNextTimeStamp) ? (nextNextNormalScheduleTimeStamp - calculatedNextTimeStamp) : (calculatedNextTimeStamp - nextNextNormalScheduleTimeStamp);

  //  Serial.print("nextDeltaAbs: ");
  //  Serial.println(nextDeltaAbs);
  //  
  //  Serial.print("nextNextDeltaAbs: ");
  //  Serial.println(nextNextDeltaAbs);


  time_t nearestNormalScheduleTimeStamp = (nextDeltaAbs < nextNextDeltaAbs) ? nextNormalScheduleTimeStamp : nextNextNormalScheduleTimeStamp;
  long nearestDeltaAbs = (nextDeltaAbs < nextNextDeltaAbs) ? nextDeltaAbs : nextNextDeltaAbs;

  //  Serial.print("nearestNormalScheduleTimeStamp: ");
  //  Serial.println(StringUtils::timeStampToString(nearestNormalScheduleTimeStamp));
  //  
  //  Serial.print("nearestDeltaAbs: ");
  //  Serial.println(nearestDeltaAbs);
  //  
  //  Serial.print("nearestDeltaAbs - WATERING_ERROR_DELTA: ");
  //  Serial.println(nearestDeltaAbs - WATERING_ERROR_DELTA);

  if (nearestDeltaAbs - WATERING_ERROR_DELTA < WATERING_MAX_SCHEDULE_CORRECTION_TIME){
    // Not all OK, but schedule without delta
    c_PumpOnAlarmIDArray[wsIndex] = c_PumpOnAlarm.triggerOnce(nearestNormalScheduleTimeStamp, turnOnWaterPumpOnSchedule);
    //         Serial.println("e");
    return;
  }

  // decrease delta on WATERING_MAX_SCHEDULE_CORRECTION_TIME but not more
  if (nearestNormalScheduleTimeStamp > calculatedNextTimeStamp){
    calculatedNextTimeStamp += WATERING_MAX_SCHEDULE_CORRECTION_TIME; 
    //        Serial.println("f");
  } 
  else {
    calculatedNextTimeStamp -= WATERING_MAX_SCHEDULE_CORRECTION_TIME;
    //     Serial.println("g");
  }
  c_PumpOnAlarmIDArray[wsIndex] = c_PumpOnAlarm.triggerOnce(calculatedNextTimeStamp, turnOnWaterPumpOnSchedule);

}

void WateringClass::turnOnWaterPumpOnSchedule(){

  showWateringMessage( F("turnOnWaterPumpOnSchedule"));

  // Find fired wsIndex
  byte wsIndex = 0xFF;
  for (byte i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++){
    if (c_PumpOnAlarmIDArray[i] == dtINVALID_ALARM_ID){
      continue;
    }
    if (c_PumpOnAlarmIDArray[i] == c_PumpOnAlarm.getTriggeredAlarmId()){
      wsIndex = i;
      break;
    }
  }
  if (wsIndex == 0xFF){
    showWateringMessage(wsIndex, F("turnOnWaterPumpOnSchedule - bad index"));
    return;
  }

  turnOnWaterPumpByIndex(wsIndex, true);

}

void WateringClass::turnOnWaterPumpByIndex(byte wsIndex, boolean isSchedulecCall){

  if (wsIndex >= MAX_WATERING_SYSTEMS_COUNT){
    return;
  }

  //showWateringMessage(wsIndex, F("turnOnWaterPumpByIndex"));

  // If already Watering - skip scheduled operation
  if (c_PumpOffAlarmIDArray[wsIndex] != dtINVALID_ALARM_ID){
    //showWateringMessage(wsIndex, F("turnOnWaterPumpByIndex - bad state"));
    return;
  }

  BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

  boolean skipRealWatering = false;

  // find watering duration by rules
  WateringEvent* wateringEvent = 0;
  byte wateringDuration = 0;

  if (isSchedulecCall){
    if (wsp.boolPreferencies.skipNextWatering){
      wsp.boolPreferencies.skipNextWatering = false;     
      skipRealWatering = true;
    } 

    if (wsp.boolPreferencies.useWetSensorForWatering){

      preUpdateWetSatus();
      updateWetSatus();

      WateringEvent* state = getCurrentWetSensorStatus(wsIndex);

      if (state == &WATERING_EVENT_WET_SENSOR_DRY){
        wateringEvent = &WATERING_EVENT_WATER_PUMP_ON_DRY;
        wateringDuration = wsp.dryWateringDuration;

      } 
      else if (state == &WATERING_EVENT_WET_SENSOR_VERY_DRY){  
        wateringEvent = &WATERING_EVENT_WATER_PUMP_ON_VERY_DRY;
        wateringDuration = wsp.veryDryWateringDuration;

      } 
      else if (state == &WATERING_EVENT_WET_SENSOR_IN_AIR ||
        state == &WATERING_EVENT_WET_SENSOR_UNSTABLE ||
        state == &WATERING_EVENT_WET_SENSOR_DISABLED){

        wateringEvent = &WATERING_EVENT_WATER_PUMP_ON_NO_SENSOR_DRY;
        wateringDuration = wsp.dryWateringDuration;
      } 
      else {
        skipRealWatering = true;
      }
    } 
    else {
      wateringEvent = &WATERING_EVENT_WATER_PUMP_ON_AUTO_DRY;
      wateringDuration = wsp.dryWateringDuration;    
    }
  } 
  else {
    //   Serial.println("!isSchedulecCall");
    wateringEvent = &WATERING_EVENT_WATER_PUMP_ON_MANUAL_DRY;
    wateringDuration = wsp.dryWateringDuration;
  }
  //  if (wateringDuration == 0){
  //    //showWateringMessage(wsIndex, F("turnOnWaterPumpByIndex - zero duration"));
  //    return;
  //  }

  if (!skipRealWatering) {
    //Serial.println("!skipRealWatering");

    // log it
    GB_Logger.logWateringEvent(wsIndex, *wateringEvent, wateringDuration);

    // Turn ON water Pump
    digitalWrite(WATERING_PUMP_PINS[wsIndex], RELAY_ON);

    // Turn OFF water Pump after delay
    c_PumpOffAlarmIDArray[wsIndex] = c_PumpOffAlarm.timerOnce(wateringDuration, turnOffWaterPumpOnSchedule);
  }

  // Schedule Next watering time
  wsp.lastWateringTimeStamp = now();
  GB_StorageHelper.setWateringSystemPreferenciesById(wsIndex, wsp);
  scheduleNextWateringTime(wsIndex);

}

void WateringClass::turnOffWaterPumpOnSchedule(){

  //showWateringMessage( F("turnOffWaterPumpByIndex"));
  //showWateringMessage(c_PumpOffAlarm.getTriggeredAlarmId());

  // Find fired wsIndex
  byte wsIndex = 0xFF;
  for (byte i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++){
    if (c_PumpOffAlarmIDArray[i] == dtINVALID_ALARM_ID){
      continue;
    }
    if (c_PumpOffAlarmIDArray[i] == c_PumpOffAlarm.getTriggeredAlarmId()){
      wsIndex = i;
      break;
    }
  }
  if (wsIndex == 0xFF){
    //showWateringMessage(wsIndex, F("turnOffWaterPumpByIndex - bad index"));
    return;
  }

  // If NOT already Watering - skip scheduled operation
  if (c_PumpOffAlarmIDArray[wsIndex] == dtINVALID_ALARM_ID){
    //showWateringMessage(wsIndex, F("turnOffWaterPumpByIndex - bad state"));
    return;
  }

  // log it
  GB_Logger.logWateringEvent(wsIndex, WATERING_EVENT_WATER_PUMP_OFF, WATERING_DISABLE_VALUE);

  // Turn ON water Pump
  digitalWrite(WATERING_PUMP_PINS[wsIndex], RELAY_OFF);

  // Turn OFF water Pump after delay
  c_PumpOffAlarmIDArray[wsIndex] = dtINVALID_ALARM_ID;

  //showWateringMessage(wsIndex, F("Turn Pump OFF"));

}


boolean WateringClass::isWetSensorValueReserved(byte value){
  return (value == WATERING_DISABLE_VALUE || value == WATERING_UNSTABLE_VALUE);
}

byte WateringClass::getCurrentWetSensorValue(byte wsIndex){
  if(wsIndex >= MAX_WATERING_SYSTEMS_COUNT){
    return WATERING_DISABLE_VALUE;
  }
  return c_lastWetSensorValue[wsIndex];
}

WateringEvent* WateringClass::getCurrentWetSensorStatus(byte wsIndex){
  if(wsIndex >= MAX_WATERING_SYSTEMS_COUNT){
    return &WATERING_EVENT_WET_SENSOR_DISABLED;
  }
  BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
  return valueToState(wsp, c_lastWetSensorValue[wsIndex]);
}

// private:

byte WateringClass::readWetValue(byte wsIndex){

  if (g_useSerialMonitor){
    showWateringMessage(wsIndex, F(""), false);
  }

  byte lastValue = 0;  
  byte equalsCounter = 0;

  for (byte counter = 0; counter < 12; counter++) {

    byte currentValue = (analogRead(WATERING_WET_SENSOR_IN_PINS[wsIndex])>>2);  
    if (g_useSerialMonitor){
      Serial.print(currentValue);
      Serial.print(' ');
    }

    if (((currentValue >= lastValue) && ((currentValue - lastValue) <= 2)) || ((lastValue > currentValue) && ((lastValue - currentValue) <= 2)) ){

      if (counter > 0){ // prevent first loop incremtnt
        equalsCounter++;
      }

      if (equalsCounter == 2){      
        if (isWetSensorValueReserved(currentValue)){
          currentValue = 2;
        }

        if (g_useSerialMonitor){
          if (isWetSensorValueReserved(currentValue)){
            Serial.print(F(" -> 2 "));
          }
          Serial.println(F("OK"));
        }
        return currentValue;
      }
    } 
    else {
      equalsCounter = 0;
    }
    lastValue = currentValue;
    delay(10);
  }
  if (g_useSerialMonitor){
    Serial.println(F("FAIL"));
  }
  return WATERING_UNSTABLE_VALUE;
}

WateringEvent* WateringClass::valueToState(const BootRecord::WateringSystemPreferencies& wsp, byte value){

  // RESERVED values
  if (value == WATERING_DISABLE_VALUE){
    return &WATERING_EVENT_WET_SENSOR_DISABLED;
  }
  else if (value == WATERING_UNSTABLE_VALUE){
    return &WATERING_EVENT_WET_SENSOR_UNSTABLE;
  } 

  // Calculated states
  else if (value > wsp.inAirValue){
    return &WATERING_EVENT_WET_SENSOR_IN_AIR;
  }
  else if (value > wsp.veryDryValue){
    return &WATERING_EVENT_WET_SENSOR_VERY_DRY;
  }
  else if (value > wsp.dryValue){
    return &WATERING_EVENT_WET_SENSOR_DRY;
  }
  else if (value > wsp.normalValue){
    return &WATERING_EVENT_WET_SENSOR_NORMAL;
  }
  else if (value > wsp.wetValue){
    return &WATERING_EVENT_WET_SENSOR_WET;
  }
  else if (value > wsp.veryWetValue){
    return &WATERING_EVENT_WET_SENSOR_VERY_WET;
  }
  else {
    return &WATERING_EVENT_WET_SENSOR_SHORT_CIRCIT;
  }
}


WateringClass GB_Watering;




