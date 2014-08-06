#include "Watering.h"

#include "StorageHelper.h"
#include "Logger.h"

byte WateringClass::analogToByte(word input){
  return (input>>2); // 10 bit in analog input
}

void WateringClass::init(){
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    c_lastWetSensorValue[wsIndex] = 0; 
  }
}

void WateringClass::turnOnWetSensors(){
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (wsp.boolPreferencies.isWetSensorConnected){
      digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], HIGH);
    }
  }
}

boolean WateringClass::updateWetStatus(){

  boolean wateringNeed = false;

  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){

    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);  
    if (wsp.boolPreferencies.isWetSensorConnected){

      byte wetValue = readWetValue(wsp, wsIndex);

      WateringEvent* oldState = valueToState(wsp, c_lastWetSensorValue[wsIndex]);
      WateringEvent* newState = valueToState(wsp, wetValue);

      if (oldState != newState){
        GB_Logger.logWateringEvent(wsIndex, *newState);  
      }
      c_lastWetSensorValue[wsIndex] = wetValue;

      if (newState == &WATERING_EVENT_WET_SENSOR_DRY || newState == &WATERING_EVENT_WET_SENSOR_VERY_DRY){
        wateringNeed = true;
      }  
    } 
    else {
      c_lastWetSensorValue[wsIndex] = 0;
    }
    digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], LOW);
  }
  return wateringNeed;
}

void WateringClass::turnOnWaterPumps(){
  if (g_useSerialMonitor){
    Serial.println(F("Turn on Water Pumps"));
  }
}

void WateringClass::updateWetSensorsForce(){
  
  boolean isUpdateNeed = false;
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);  
    if (wsp.boolPreferencies.isWetSensorConnected){
      isUpdateNeed = true;
      break;  
    }
  }
  if(!isUpdateNeed){
    return;
  }

  turnOnWetSensors();
  delay(WATERING_SYSTEM_TURN_ON_DELAY*1000);
  updateWetStatus();
}

byte WateringClass::getCurrentWetSensorValue(byte wsIndex){
  if(wsIndex >= MAX_WATERING_SYSTEMS_COUNT){
    return 0;
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
byte WateringClass::readWetValue(const BootRecord::WateringSystemPreferencies& wsp, byte wsIndex){

  if (g_useSerialMonitor){
    showWateringMessage(wsIndex, F(""), false);
  }

  byte currentValue = analogToByte(analogRead(WATERING_WET_SENSOR_IN_PINS[wsIndex]));
  if (g_useSerialMonitor){
    Serial.print(currentValue);
    Serial.print(' ');
  }
  byte lastValue = currentValue;
  byte equalsCounter = 1;
  for(byte counter = 0; counter < 12; counter++) {

    currentValue = analogToByte(analogRead(WATERING_WET_SENSOR_IN_PINS[wsIndex]));  
    if (g_useSerialMonitor){
      Serial.print(currentValue);
      Serial.print(' ');
    }

    if (currentValue == lastValue || ((currentValue > lastValue) && ((currentValue - lastValue) < 2)) || ((lastValue > currentValue) && ((lastValue - currentValue) < 2)) ){
      equalsCounter++;
      if (equalsCounter == 3){
        if (g_useSerialMonitor){
          if (currentValue == 0){
            Serial.println(F(" -> 1 "));
          }
          Serial.println(F("OK"));
        }
        if (currentValue == 0){
          currentValue = 1; // minimumm 1, 0 used fir Disabled state
        }
        return currentValue;
      }
    } 
    else {
      equalsCounter = 1;
    }
    lastValue = currentValue;
    delay(10);
  }
  if (g_useSerialMonitor){
    Serial.println(F("FAIL"));
  }
  return 0; // TODO implement unstable
}

WateringEvent* WateringClass::valueToState(const BootRecord::WateringSystemPreferencies& wsp, byte value){

  if (value == 0){
    return &WATERING_EVENT_WET_SENSOR_DISABLED;
  }
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








