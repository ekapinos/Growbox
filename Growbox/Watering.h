#ifndef Watering_h
#define Watering_h

#include <Time.h>
// We use free() method
#define USE_SPECIALIST_METHODS 
#include <TimeAlarms.h>

#include "Global.h"
#include "LoggerModel.h"
#include "StorageModel.h"

class WateringClass{
private:

  static const byte WATERING_DISABLE_VALUE = 0;
  static const byte WATERING_UNSTABLE_VALUE = 1;
  
  static time_t c_turnOnWetSensorsTimeStamp;
  static byte c_lastWetSensorValue[MAX_WATERING_SYSTEMS_COUNT];

  static TimeAlarmsClass c_PumpOnAlarm;
  static TimeAlarmsClass c_PumpOffAlarm;
  static AlarmID_t c_PumpOnAlarmIDArray[MAX_WATERING_SYSTEMS_COUNT];
  static AlarmID_t c_PumpOffAlarmIDArray[MAX_WATERING_SYSTEMS_COUNT];

public:

  static void init(time_t turnOnWetSensorsTime);
  static void updateAlarms();
  
  static void adjustWatringTimeOnClockSet(long);
  
  /////////////////////////////////////////////////////////////////////
  //                           WET SENSORS                           //
  /////////////////////////////////////////////////////////////////////

  static boolean preUpdateWetSatus();
  static void updateWetSatus();

  /////////////////////////////////////////////////////////////////////
  //                           WATER PUMPS                           //
  /////////////////////////////////////////////////////////////////////
  static void updateWateringSchedule();
    
  static time_t getLastWateringTimeStampByIndex(byte wsIndex);
  static time_t getNextWateringTimeStampByIndex(byte wsIndex);
  
  static void turnOnWaterPumpManual(byte wsIndex);
private:
  static void scheduleNextWateringTime(byte wsIndex);

  static void turnOnWaterPumpOnSchedule(); 
  static void turnOnWaterPumpByIndex(byte wsIndex, boolean isScheduleCall);
  static void turnOffWaterPumpOnSchedule();

public:

  static boolean isWetSensorValueReserved(byte value);
  static byte getCurrentWetSensorValue(byte wsIndex);
  static WateringEvent* getCurrentWetSensorStatus(byte wsIndex);

private:

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



