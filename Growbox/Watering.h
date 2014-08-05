#ifndef Watering_h
#define Watering_h

#include "Global.h"
#include "LoggerModel.h"
#include "StorageModel.h"

class WateringClass{
public:

private:
  byte c_lastWetSensorValue[MAX_WATERING_SYSTEMS_COUNT];
 
public:
  byte analogToByte(word input);
  
  void init();
  
  void turnOnWetSensors();
  boolean updateWetStatus();
  void turnOnWaterPumps();
  
  void updateWetSensorsForce();
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
