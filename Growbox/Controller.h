#ifndef Controller_h
#define Controller_h

#include "Global.h"

class ControllerClass{

public:

  void rebootController();

  // discover memory overflow errors in the arduino C++ code
  void checkFreeMemory();
  
  void updateSerialMonitorStatus();

};

extern ControllerClass GB_Controller;

#endif

