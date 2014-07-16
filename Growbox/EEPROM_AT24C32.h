#ifndef EEPROM_AT24C32_h
#define EEPROM_AT24C32_h

#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif

#include <Wire.h>

class EEPROM_AT24C32_Class {

private:

  static const int I2C_ADDRESS = 0x50; // External EEPROM I2C address

public:

  static const word CAPACITY = 0x1000; // 4K byte = 32K bit

  boolean isPresent(); // check if the device is present

  void write(const word address, const byte data) ;
  byte read(word address);

  // Templates copied from:
  //
  //  EEPROMEx.h - Extended EEPROM library
  //  Copyright (c) 2012 Thijs Elenbaas.  All right reserved.

  // Use template for other data formats
  template <class T> int readBlock(const word address, const T value[], int items)
  {
    if (!isWriteOk(address+items*sizeof(T))) return 0;
    unsigned int i;
    for (i = 0; i < (unsigned int)items; i++)
      readBlock<T>(address+(i*sizeof(T)),value[i]);
    return i;
  }

  template <class T> int readBlock(const word address, const T& value)
  {		
    read_block(address, (void*)&value, sizeof(value));
    return sizeof(value);
  }

  template <class T> int writeBlock(const word address, const T value[], int items)
  {	
    if (!isWriteOk(address+items*sizeof(T)-1)) return 0;
    unsigned int i;
    for (i = 0; i < (unsigned int)items; i++)
      writeBlock<T>(address+(i*sizeof(T)),value[i]);
    return i;
  }

  template <class T> int writeBlock(const word address, const T& value)
  {
    if (!isWriteOk(address+sizeof(value)-1)) return 0;
    write_block((void*)&value, (void*)address, sizeof(value));			  			  
    return sizeof(value);
  }

  template <class T> int updateBlock(const word address, const T value[], int items)
  {
    int writeCount=0;
    if (!isWriteOk(address+items*sizeof(T)-1)) return 0;
    unsigned int i;
    for (i = 0; i < (unsigned int)items; i++)
      writeCount+= updateBlock<T>(address+(i*sizeof(T)),value[i]);
    return writeCount;
  }

  template <class T> int updateBlock(word address, const T& value)
  {
    int writeCount=0;
    if (!isWriteOk(address+sizeof(value)-1)) return 0;
    const byte* bytePointer = (const byte*)(const void*)&value;
    for (unsigned int i = 0; i < (unsigned int)sizeof(value); i++) {
      if (read(address)!=*bytePointer) {
        write(address, *bytePointer);
        writeCount++;		
      }
      address++;
      bytePointer++;
    }
    return writeCount;
  }
private:
  boolean isWriteOk(const word address);
  
  void write_block(const word address, const void* data, const byte sizeofData) ;
  void read_block(const word address, void *data, const byte sizeofData);

  /////////////////////////////////////////////////////////////////////
  //                          EXTRA COMMANDS                         //
  /////////////////////////////////////////////////////////////////////

  void fillStorage(byte value);
  void fillStorageIncremental();

}; 

extern EEPROM_AT24C32_Class EEPROM_AT24C32;

#endif




