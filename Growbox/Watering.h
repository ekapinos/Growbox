#ifndef Watering_h
#define Watering_h

#include <Time.h>
#include <TimeAlarms.h>

#include "Global.h"
#include "LoggerModel.h"
#include "StorageModel.h"

class WateringClass{
public:

  const static byte WATERING_DISABLE_VALUE = 0;
  const static byte WATERING_UNSTABLE_VALUE = 1;

private:
  static TimeAlarmsClass* c_Alarm;
  time_t c_turnOnWetSensorsTime;
  byte c_lastWetSensorValue[MAX_WATERING_SYSTEMS_COUNT];
  static byte c_waterPumpDisableTimeout[MAX_WATERING_SYSTEMS_COUNT];

public:

  void init(time_t turnOnWetSensorsTime, TimeAlarmsClass* AlarmForWatering);

  boolean turnOnWetSensors();
  boolean updateWetStatus();
  
  static boolean isWaterPumpsTurnedOn();
  void turnOnWaterPumps();
  static void turnOnWaterPumpForce(byte wsIndex);
  

  boolean isWetSensorValueReserved(byte value);
  byte getCurrentWetSensorValue(byte wsIndex);
  WateringEvent* getCurrentWetSensorStatus(byte wsIndex);

private:
  static void turnOffWaterPumpsOnSchedule();
  byte readWetValue(byte wsIndex);
  WateringEvent* valueToState(const BootRecord::WateringSystemPreferencies& wsp, byte input);


  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////

  template <class T> static void showWateringMessage(byte wsIndex, T str, boolean newLine = true){
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

  template <class T> static void showWateringMessage(T str, boolean newLine = true){
    if (g_useSerialMonitor){
      Serial.print(F("WATERING> "));
      Serial.print(str);
      if (newLine){  
        Serial.println();
      }      
    }
  }
};

extern WateringClass GB_Watering;

#endif

