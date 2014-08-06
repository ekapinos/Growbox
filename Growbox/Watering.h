#ifndef Watering_h
#define Watering_h

#include "Global.h"
#include "LoggerModel.h"
#include "StorageModel.h"

class WateringClass{
public:

const static byte WATERING_DISABLE_VALUE = 0;
const static byte WATERING_UNSTABLE_VALUE = 1;

private:
  byte c_lastWetSensorValue[MAX_WATERING_SYSTEMS_COUNT];
  byte c_waterPumpDisableTimeout[MAX_WATERING_SYSTEMS_COUNT];
 
public:
  
  void init();
    
  void updateWetStatusForce();
  
  boolean turnOnWetSensors();
  boolean updateWetStatus();
  byte turnOnWaterPumps();
  byte turnOffWaterPumpsOnSchedule();
  
  boolean isWetSensorValueReserved(byte value);
  byte getCurrentWetSensorValue(byte wsIndex);
  WateringEvent* getCurrentWetSensorStatus(byte wsIndex);

private:
  byte readWetValue(const BootRecord::WateringSystemPreferencies& wsp, byte wsIndex);
  WateringEvent* valueToState(const BootRecord::WateringSystemPreferencies& wsp, byte input);

  
  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////

  template <class T> void showWateringMessage(byte wsIndex, T str, boolean newLine = true){
    if (g_useSerialMonitor){
      Serial.print(F("WATERING> ["));
      
      Serial.print(wsIndex+1);
      Serial.print(F("] "));
      Serial.print(str);
      if (newLine){  
        Serial.println();
      }      
    }
  }
};

extern WateringClass GB_Watering;

#endif
