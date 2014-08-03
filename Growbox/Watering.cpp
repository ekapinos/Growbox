#include "Watering.h"

#include "StorageHelper.h"

byte WateringClass::analogToByte(word input){
  return (input>>2); // 10 bit in analog input
}

void WateringClass::init(){

  for (byte i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++){
    BootRecord::WateringSystemPreferencies wsp = GB_StorageHelper.getWateringSystemPreferenciesById(i);
    if (wsp.boolPreferencies.isWetSensorConnected){
      c_lastWetSensorState[i] = readWetState(wsp, i);
    }
    digitalWrite(WATERING_WET_SENSOR_POWER_PINS[i], LOW);
  }
}

WateringClass::WetSensorState WateringClass::analogToState(const BootRecord::WateringSystemPreferencies& wsp, word input){

  byte byteInput = analogToByte(input);

  if (wsp.inAirValue > byteInput){
    return WET_SENSOR_IN_AIR;
  }
  else if (wsp.veryDryValue > byteInput){
    return WET_SENSOR_VERY_DRY;
  }
  else if (wsp.dryValue > byteInput){
    return WET_SENSOR_DRY;
  }
  else if (wsp.normalValue > byteInput){
    return WET_SENSOR_NORMAL;
  }
  else if (wsp.wetValue > byteInput){
    return WET_SENSOR_WET;
  }
  else if (wsp.veryWetValue > byteInput){
    return WET_SENSOR_VERY_WET;
  }
  else {
    return WET_SENSOR_SHORT_CIRCIT;
  }
}

WateringClass::WetSensorState WateringClass::readWetState(const BootRecord::WateringSystemPreferencies& wsp, byte pin){
  
  word lastValue = analogRead(WATERING_WET_SENSOR_IN_PINS[pin]);
  word currentValue;
  byte equalsCounter = 0;
  for(byte counter = 0; counter < 12; counter++) {
    delay(10);
    currentValue = analogRead(WATERING_WET_SENSOR_IN_PINS[pin]);
    if (currentValue == lastValue){
      equalsCounter++;
      if (equalsCounter == 3){
        return analogToState(wsp, currentValue);
      }
    } else {
      equalsCounter = 0;
    }
    lastValue == currentValue;
  }
  
  return WET_SENSOR_UNKNOWN;
}

WateringClass GB_Watering;

