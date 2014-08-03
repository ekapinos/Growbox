#ifndef Watering_h
#define Watering_h

#include "Global.h"
#include "StorageModel.h"

class WateringClass{
public:

  enum WetSensorState {
    WET_SENSOR_IN_AIR,
    WET_SENSOR_VERY_DRY,
    WET_SENSOR_DRY,
    WET_SENSOR_NORMAL,
    WET_SENSOR_WET,
    WET_SENSOR_VERY_WET,
    WET_SENSOR_SHORT_CIRCIT,
    WET_SENSOR_UNKNOWN // can't read wet value, readings are not stable
  };

private:
  WetSensorState c_lastWetSensorState[MAX_WATERING_SYSTEMS_COUNT];
 
public:
  byte analogToByte(word input);
  
  void init();
  
private:
  WetSensorState analogToState(const BootRecord::WateringSystemPreferencies& wsp, word input);
  WetSensorState readWetState(const BootRecord::WateringSystemPreferencies& wsp, byte pin);

};

extern WateringClass GB_Watering;

#endif
