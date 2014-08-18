#ifndef Controller_h
#define Controller_h

#include "Global.h"

class ControllerClass{

  int c_freeMemoryLastCheck;
  boolean c_isAutoCalculatedTimeStampUsed;
public:
  ControllerClass();

  void rebootController();

  // discover memory overflow errors in the arduino C++ code
  void checkInputPinsStatus(boolean checkFirmwareReset = false);
  void checkFreeMemory();
  void update();

  void initClock(time_t defaultTimeStamp);
  void initClock_afterLoadConfiguration();

  void setClockTime(time_t newTimeStamp);
private:
  void setHarwareAndSoftwareClockTimeStamp(time_t newTimeStamp);
public:
  //  void updateClockState();
  boolean isHardwareClockPresent();
  //boolean isAutoCalculatedTimeUsed();
  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////

  template <class T> void showControllerMessage(T str, boolean newLine = true){
    if (g_useSerialMonitor){
      Serial.print(F("CONTROLLER> "));
      Serial.print(str);
      if (newLine){  
        Serial.println();
      }      
    }
  }
};

extern ControllerClass GB_Controller;

#endif






