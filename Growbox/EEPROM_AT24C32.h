#ifndef EEPROM_AT24C32_h
#define EEPROM_AT24C32_h

#include "EEPROM_ARDUINO.h"

class EEPROM_AT24C32_Class : public EEPROM_ARDUINO_Class{

public:

  virtual word getCapacity();
  virtual boolean isPresent(); // check if the device is present

private:

  virtual void write_byte(const word address, const byte data);
  virtual byte read_byte(word address);
  virtual void write_block(const word address, const void* data, const word sizeofData);
  virtual void read_block(const word address, void *data, const word sizeofData);

};

extern EEPROM_AT24C32_Class EEPROM_AT24C32;

#endif

