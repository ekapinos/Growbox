#include "EEPROM_ARDUINO.h"

#include <avr/eeprom.h>

#define EEPROMSizeATmega168   512     
#define EEPROMSizeATmega328   1024     
#define EEPROMSizeATmega1280  4096     
#define EEPROMSizeATmega32u4  1024
#define EEPROMSizeAT90USB1286 4096
#define EEPROMSizeMK20DX128   2048

#define EEPROMSizeUno         EEPROMSizeATmega328     
#define EEPROMSizeUnoSMD      EEPROMSizeATmega328
#define EEPROMSizeLilypad     EEPROMSizeATmega328
#define EEPROMSizeDuemilanove EEPROMSizeATmega328
#define EEPROMSizeMega        EEPROMSizeATmega1280
#define EEPROMSizeDiecimila   EEPROMSizeATmega168
#define EEPROMSizeNano        EEPROMSizeATmega168
#define EEPROMSizeTeensy2     EEPROMSizeATmega32u4
#define EEPROMSizeLeonardo    EEPROMSizeATmega32u4
#define EEPROMSizeMicro       EEPROMSizeATmega32u4
#define EEPROMSizeYun         EEPROMSizeATmega32u4
#define EEPROMSizeTeensy2pp   EEPROMSizeAT90USB1286
#define EEPROMSizeTeensy3     EEPROMSizeMK20DX128

#ifndef ARDUINO_CAPACITY
// Define your device before import
#define ARDUINO_CAPACITY       EEPROMSizeMega
#endif

word EEPROM_ARDUINO_Class::getCapacity() {
  return ARDUINO_CAPACITY;
}

boolean EEPROM_ARDUINO_Class::isPresent() {
  return eeprom_is_ready();
}

void EEPROM_ARDUINO_Class::write(const word address, const byte data) {
  if (address >= getCapacity()) {
    return;
  }
  write_byte(address, data);
}

byte EEPROM_ARDUINO_Class::read(word address) {
  if (address >= getCapacity()) {
    return 0;
  }
  return read_byte(address);
}

/////////////////////////////////////////////////////////////////////
//                             TESTING                             //
/////////////////////////////////////////////////////////////////////

void EEPROM_ARDUINO_Class::fillWithValue(byte value) {
  for (word i = 0; i < getCapacity(); i++) {
    write(i, value);
  }
}

void EEPROM_ARDUINO_Class::fillIncremental() {
  for (word i = 0; i < getCapacity(); i++) {
    write(i, (byte)i);
  }
}

/////////////////////////////////////////////////////////////////////
//                             TESTING                             //
/////////////////////////////////////////////////////////////////////

void EEPROM_ARDUINO_Class::write_byte(const word address, const byte data) {
  eeprom_write_byte((uint8_t*)address, data);
}

byte EEPROM_ARDUINO_Class::read_byte(const word address) {
  return eeprom_read_byte((uint8_t*)address);
}

void EEPROM_ARDUINO_Class::write_block(const word address, const void* data, const word sizeofData) {
  eeprom_write_block(data, (uint8_t*)address, sizeofData);
}

void EEPROM_ARDUINO_Class::read_block(const word address, void *data, const word sizeofData) {
  eeprom_read_block(data, (uint8_t*)address, sizeofData);
}

EEPROM_ARDUINO_Class EEPROM;

