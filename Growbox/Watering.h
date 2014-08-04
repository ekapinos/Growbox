#ifndef Watering_h
#define Watering_h

#include "Global.h"
#include "LoggerModel.h"
#include "StorageModel.h"

class WateringClass{
public:

private:
  WateringEvent* c_lastWetSensorState[MAX_WATERING_SYSTEMS_COUNT];
 
public:
  byte analogToByte(word input);
  
  void init();
  
  void turnOnWetSensors();
  void updateWetStatus();
  
private:
  WateringEvent* analogToState(const BootRecord::WateringSystemPreferencies& wsp, byte input);
  WateringEvent* readWetState(const BootRecord::WateringSystemPreferencies& wsp, byte wsIndex);

  
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
