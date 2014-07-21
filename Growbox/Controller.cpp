#include <MemoryFree.h>

#include "Controller.h"
#include "Logger.h"
#include "StorageHelper.h"

void ControllerClass::rebootController() {
  void(* resetFunc) (void) = 0; // Reset MC function
  resetFunc(); // call
}

void ControllerClass::checkFreeMemory(){

  int currentFreeMemory = freeMemory();
  if(currentFreeMemory< 200){ 
    GB_Logger.logError(ERROR_MEMORY_LOW);   
  } 
  else {
    GB_Logger.stopLogError(ERROR_MEMORY_LOW); 
  }
  if (g_useSerialMonitor){
    Serial.print(F("GB> Free memory: ["));  
    Serial.print(currentFreeMemory);
    Serial.println(']');
  }
}

void ControllerClass::updateHardwareInputStatus(){

  boolean oldUseSerialMonitor  = g_useSerialMonitor;

  g_useSerialMonitor = (digitalRead(HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN) == HARDWARE_BUTTON_ON);

  if (oldUseSerialMonitor != g_useSerialMonitor){

    if (g_useSerialMonitor){
      Serial.begin(9600);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
      } 
      showControllerMessage(F("Serial monitor enabled"));
    } 
    else {
      showControllerMessage(F("Serial monitor disabled"));
      Serial.end();
    }
  }

  if (g_useSerialMonitor && digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON){ 
    showControllerMessage("Reset firmware...");
    byte counter;
    for (counter = 5; counter>0; counter--){
      Serial.println(counter);
      delay(1000);
      if (digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) != HARDWARE_BUTTON_ON){
        break;
      }
    }
    if (counter == 0){
      GB_StorageHelper.resetFirmware();
      showControllerMessage("Operation finished succesfully. Remove wire from Reset pin. Device will reboot automatically");
      while(digitalRead(HARDWARE_BUTTON_RESET_FIRMWARE_PIN) == HARDWARE_BUTTON_ON) {
        delay(1000);
      }
      rebootController();
    } 
    else {
      showControllerMessage("Operation aborted");
    }
  }
}

ControllerClass GB_Controller;






