#ifndef GB_Controller_h
#define GB_Controller_h

#include "Global.h"

class GB_Controller{
public:
  static void rebootController() {
    if (g_UseSerialMonitor) {
      Serial.println(F("Rebooting..."));
    }
    void(* resetFunc) (void) = 0; // Reset MC function
    resetFunc(); //call
  }
};

#endif

