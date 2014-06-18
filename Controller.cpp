#include "Controller.h"

#include "Global.h"

void rebootController() {
  if (g_UseSerialMonitor) {
    Serial.println(F("Rebooting..."));
  }
  void(* resetFunc) (void) = 0; // Reset MC function
  resetFunc(); //call
}

