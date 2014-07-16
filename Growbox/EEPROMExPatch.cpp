#include "EEPROMExPatch.h"

boolean EEPROMClassExPatch::isWriteOk(const word address){
  return (address<=EEPROMSizeMega); 
}

boolean EEPROMClassExPatch::isReady() {
  return eeprom_is_ready();
}

EEPROMClassExPatch EEPROM;

