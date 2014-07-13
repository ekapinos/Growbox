#ifndef AT24C32_EEPROM_h
#define AT24C32_EEPROM_h

#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif

#include <Wire.h>

class AT24C32_EEPROM_Class {
  
private:

  static const int I2C_ADDRESS = 0x50; // External EEPROM I2C address

public:

  static const word CAPACITY = 0x1000; // 4K byte = 32K bit

  boolean isPresent(); // check if the device is present

  void write(const word address, const byte data) ;
  byte read(word address);

  void write(word address, const void* data, const byte sizeofData) ;
  void read(word address, void *data, const byte sizeofData);

  /////////////////////////////////////////////////////////////////////
  //                          EXTRA COMMANDS                         //
  /////////////////////////////////////////////////////////////////////

  void fillStorage(byte value);
  void fillStorageIncremental();

}; 

extern AT24C32_EEPROM_Class AT24C32_EEPROM;

#endif



