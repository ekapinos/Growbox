#include "Watering.h"

#include "StorageHelper.h"
#include "Logger.h"

void WateringClass::init(time_t turnOnWetSensorsTime){

  if(g_useSerialMonitor){
    showWateringMessage(F("init - start, at "), false);
    Serial.println(StringUtils::timeStampToString(turnOnWetSensorsTime));
  }

  c_turnOnWetSensorsTime = turnOnWetSensorsTime;

  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    c_lastWetSensorValue[wsIndex] = WATERING_DISABLE_VALUE; 

    // Turn OFF unused Wet sensor pins
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (!wsp.boolPreferencies.isWetSensorConnected){
      digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], LOW);
      showWateringMessage(wsIndex, F("Wet sensor OFF"));
    }
  }

  showWateringMessage(F("init - stop"));
}

boolean WateringClass::turnOnWetSensors(){

  showWateringMessage(F("Turn ON sensors - start"));

  if (c_turnOnWetSensorsTime != 0){
    showWateringMessage(F("Skiped"));
    showWateringMessage(F("Turn ON sensors - stop"));
    return true;
  }

  c_turnOnWetSensorsTime = now();

  boolean isSensorsTurnedOn = false;
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (wsp.boolPreferencies.isWetSensorConnected){
      showWateringMessage(wsIndex, F("Wet sensor ON"));
      digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], HIGH);
      isSensorsTurnedOn = true;
    }
    else {
      digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], LOW);
      if (c_lastWetSensorValue[wsIndex] != WATERING_DISABLE_VALUE){
        c_lastWetSensorValue[wsIndex] = WATERING_DISABLE_VALUE;
        GB_Logger.logWateringEvent(wsIndex, WATERING_EVENT_WET_SENSOR_DISABLED);  
      }
    }
  }

  showWateringMessage(F("Turn ON sensors - stop"));

  return isSensorsTurnedOn;
}

boolean WateringClass::updateWetStatus(){

  showWateringMessage(F("Update Wet status - start"));

  if (c_turnOnWetSensorsTime == 0){
    turnOnWetSensors();
  }  

  long remanedDelay = c_turnOnWetSensorsTime - now() + WATERING_SYSTEM_TURN_ON_DELAY;
  if (remanedDelay > 0){
    if(g_useSerialMonitor){
      showWateringMessage(F("delay = "), false);
      Serial.println(remanedDelay);
    }
    delay((unsigned long)remanedDelay * 1000);
  }
  c_turnOnWetSensorsTime = 0;

  boolean isWateringNeed = false;

  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){

    // Maybe Wet sensor was already updated (between scheduled call turnOnWetSensors and updateWetStatus was WEB call with force update) 
    if (digitalRead(WATERING_WET_SENSOR_POWER_PINS[wsIndex]) == HIGH){

      byte wetValue = readWetValue(wsIndex);
      digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], LOW);
      showWateringMessage(wsIndex, F("Wet sensor OFF"));

      BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);

      WateringEvent* oldState = valueToState(wsp, c_lastWetSensorValue[wsIndex]);
      WateringEvent* newState = valueToState(wsp, wetValue);
      showWateringMessage(wsIndex, newState->shortDescription);
      if (oldState != newState){
        GB_Logger.logWateringEvent(wsIndex, *newState);  
      }

      c_lastWetSensorValue[wsIndex] = wetValue;

      if (newState == &WATERING_EVENT_WET_SENSOR_DRY || newState == &WATERING_EVENT_WET_SENSOR_VERY_DRY){
        isWateringNeed = true;
      }  
    }

  }

  showWateringMessage(F("Update Wet status - stop"));

  return isWateringNeed;
}

byte WateringClass::turnOnWaterPumps(){

  showWateringMessage(F("Turn on Water Pumps - start"));

  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    if (c_waterPumpDisableTimeout[wsIndex] != 0){
      showWateringMessage(F("ABORTED (previous Watering operation is not finished)"));
      showWateringMessage(F("Turn on Water Pumps - stop"));
      return 0;
    }
  }


  byte minDuration = 0;
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){

    c_waterPumpDisableTimeout[wsIndex] = 0;

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (!wsp.boolPreferencies.isWetSensorConnected || !wsp.boolPreferencies.isWaterPumpConnected){
      continue;
    }

    byte wateringDuration;
    WateringEvent* state = valueToState(wsp, c_lastWetSensorValue[wsIndex]); 
    if (state == &WATERING_EVENT_WET_SENSOR_DRY){
      wateringDuration = wsp.dryWateringDuration;
      if (wateringDuration == 0){
        continue;
      }
      GB_Logger.logWateringEvent(wsIndex, WATERING_EVENT_WATER_PUMP_ON_DRY); 

    } 
    else if (state == &WATERING_EVENT_WET_SENSOR_VERY_DRY){
      wateringDuration = wsp.veryDryWateringDuration;
      if (wateringDuration == 0){
        continue;
      }
      GB_Logger.logWateringEvent(wsIndex, WATERING_EVENT_WATER_PUMP_ON_VERY_DRY);

    }
    else {
      continue;
    }

    if(g_useSerialMonitor){
      showWateringMessage(wsIndex, F("Watering during "), false);
      Serial.print(wateringDuration);
      Serial.println( F(" sec started"));
    }

    digitalWrite(WATERING_PUMP_PINS[wsIndex], RELAY_ON);

    c_waterPumpDisableTimeout[wsIndex] = wateringDuration;

    if (minDuration == 0 || minDuration > wateringDuration){
      minDuration = wateringDuration;
    }
  }  

  if(g_useSerialMonitor){
    showWateringMessage(F("Turn on Water Pumps - stop, next duration = "), false);
    Serial.println(minDuration);
  }

  return minDuration;
}

byte WateringClass::turnOffWaterPumpsOnSchedule(){

  showWateringMessage(F("Turn OFF Water Pumps - start"));

  byte minDuration = 0;
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    if(c_waterPumpDisableTimeout[wsIndex] == 0){
      continue;
    }

    if (minDuration == 0 || minDuration > c_waterPumpDisableTimeout[wsIndex]){
      minDuration = c_waterPumpDisableTimeout[wsIndex];
    }
  }

  byte nextMinDuration = 0;
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    if(c_waterPumpDisableTimeout[wsIndex] == 0){
      continue;
    }

    c_waterPumpDisableTimeout[wsIndex] -= minDuration;

    if (c_waterPumpDisableTimeout[wsIndex] == 0){

      digitalWrite(WATERING_PUMP_PINS[wsIndex], RELAY_OFF);
      if(g_useSerialMonitor){
        showWateringMessage(wsIndex, F("Watering finished"));
      }

      continue;
    } 

    if (nextMinDuration == 0 || nextMinDuration > c_waterPumpDisableTimeout[wsIndex]){
      nextMinDuration = c_waterPumpDisableTimeout[wsIndex];  
    }
  } 

  showWateringMessage(F("Turn OFF Water Pumps - stop, next duration = "), false);
  Serial.println(nextMinDuration);

  return nextMinDuration;
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
            Serial.println(F(" -> 2 "));
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
















