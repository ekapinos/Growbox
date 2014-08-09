#ifndef Watering_h
#define Watering_h

#include <Time.h>
#include <TimeAlarms.h>

#include "Global.h"
#include "LoggerModel.h"
#include "StorageModel.h"

class WateringClass{
public:

  static const byte WATERING_DISABLE_VALUE = 0;
  static const byte WATERING_UNSTABLE_VALUE = 1;

private:
  static TimeAlarmsClass c_Alarm;
  static time_t c_turnOnWetSensorsTime;
  static byte c_lastWetSensorValue[MAX_WATERING_SYSTEMS_COUNT];
  static byte c_waterPumpDisableTimeout[MAX_WATERING_SYSTEMS_COUNT];

public:

  static void init(time_t turnOnWetSensorsTime);
  static void updateInternalAlarm();

  static boolean turnOnWetSensors();
  static boolean updateWetStatus();
  
  static boolean isWaterPumpsTurnedOn();
  static void turnOnWaterPumps();
  static void turnOnWaterPumpForce(byte wsIndex);
  

  static boolean isWetSensorValueReserved(byte value);
  static byte getCurrentWetSensorValue(byte wsIndex);
  static WateringEvent* getCurrentWetSensorStatus(byte wsIndex);

private:
  static void turnOffWaterPumpsOnSchedule();
  static byte readWetValue(byte wsIndex);
  static WateringEvent* valueToState(const BootRecord::WateringSystemPreferencies& wsp, byte input);


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

