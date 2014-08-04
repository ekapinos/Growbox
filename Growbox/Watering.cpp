#include "Watering.h"

#include "StorageHelper.h"
#include "Logger.h"

byte WateringClass::analogToByte(word input){
  return (input>>2); // 10 bit in analog input
}

void WateringClass::init(){
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    c_lastWetSensorState[wsIndex] = 0; // NULL pointer
  }
}

void WateringClass::turnOnWetSensors(){
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (wsp.boolPreferencies.isWetSensorConnected){
      digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], HIGH);
      if (g_useSerialMonitor){
        showWateringMessage(wsIndex, F("Wet sensor ON"));
      }
    }
  }
}

void WateringClass::updateWetStatus(){
  for (byte wsIndex = 0; wsIndex < MAX_WATERING_SYSTEMS_COUNT; wsIndex++){
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(wsIndex);
    if (wsp.boolPreferencies.isWetSensorConnected){
      WateringEvent* newState = readWetState(wsp, wsIndex);
      if (newState != c_lastWetSensorState[wsIndex]){
        GB_Logger.logWateringEvent(wsIndex, *newState);
        c_lastWetSensorState[wsIndex] = newState;
      }
      if (g_useSerialMonitor){
        showWateringMessage(wsIndex, F("Wet sensor OFF"));
      }
    }
    digitalWrite(WATERING_WET_SENSOR_POWER_PINS[wsIndex], LOW);
  }
}

// private:

WateringEvent* WateringClass::readWetState(const BootRecord::WateringSystemPreferencies& wsp, byte wsIndex){

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

    if (currentValue == lastValue){
      equalsCounter++;
      if (equalsCounter == 3){
        if (g_useSerialMonitor){
          Serial.println(F("OK"));
        }
        return analogToState(wsp, currentValue);
      }
    } 
    else {
      equalsCounter = 1;
      lastValue = currentValue;
    }
    delay(10);
  }
  if (g_useSerialMonitor){
    Serial.println(F("FAIL"));
  }
  return &WATERING_EVENT_WET_SENSOR_UNKNOWN;
}

WateringEvent* WateringClass::analogToState(const BootRecord::WateringSystemPreferencies& wsp, byte input){

  if (input > wsp.inAirValue){
    return &WATERING_EVENT_WET_SENSOR_IN_AIR;
  }
  else if (input > wsp.veryDryValue){
    return &WATERING_EVENT_WET_SENSOR_VERY_DRY;
  }
  else if (input > wsp.dryValue){
    return &WATERING_EVENT_WET_SENSOR_DRY;
  }
  else if (input > wsp.normalValue){
    return &WATERING_EVENT_WET_SENSOR_NORMAL;
  }
  else if (input > wsp.wetValue){
    return &WATERING_EVENT_WET_SENSOR_WET;
  }
  else if (input > wsp.veryWetValue){
    return &WATERING_EVENT_WET_SENSOR_VERY_WET;
  }
  else {
    return &WATERING_EVENT_WET_SENSOR_SHORT_CIRCIT;
  }
}


WateringClass GB_Watering;





