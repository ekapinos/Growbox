#include "EEPROM_ARDUINO.h"

#include <avr/eeprom.h>

boolean EEPROM_ARDUINO_Class::isWriteOk(const word address){
  return (address<CAPACITY); 
}

boolean EEPROM_ARDUINO_Class::isReady() {
  return eeprom_is_ready();
}

void EEPROM_ARDUINO_Class::write(const word address, const byte data) {
  eeprom_write_byte((uint8_t*) address, data);
}

byte EEPROM_ARDUINO_Class::read(const word address){
  return eeprom_read_byte((uint8_t*) address);
}

void EEPROM_ARDUINO_Class::write_block(const word address, const void* data, const word sizeofData) {
  eeprom_write_block(data, (uint8_t*)address, sizeofData);	
}

void EEPROM_ARDUINO_Class::read_block(const word address, void *data, const word sizeofData){
  eeprom_read_block(data, (uint8_t*)address, sizeofData);
}

EEPROM_ARDUINO_Class EEPROM;


