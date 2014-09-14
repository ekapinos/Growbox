#ifndef EEPROM_ARDUINO_h
#define EEPROM_ARDUINO_h

// Copied and adapted from: https://github.com/thijse/Arduino-Libraries/blob/master/EEPROMEx
//
//  EEPROMEx.h - Extended EEPROM library
//  Copyright (c) 2012 Thijs Elenbaas.  All right reserved.
#include <Arduino.h> 

class EEPROM_ARDUINO_Class{

public:
  virtual ~EEPROM_ARDUINO_Class() {}
  
  virtual word getCapacity();
  virtual boolean isPresent(); // check if the device is present
  
  virtual void write(const word address, const byte data) ;
  virtual byte read(word address);  
  
  /////////////////////////////////////////////////////////////////////
  //                             TESTING                             //
  /////////////////////////////////////////////////////////////////////

  void fillWithValue(byte value);
  void fillIncremental();
  
  /////////////////////////////////////////////////////////////////////
  //                            TEMPLATES                            //
  /////////////////////////////////////////////////////////////////////

  template <class T> void readBlock(const word address, T value[], word items){
    if ((address+items*sizeof(T)-1) >= getCapacity()) {
      return;
    }
    for (word i = 0; i < items; i++) {
      value[i] = readBlock<T>(address+i*sizeof(T)); 
    }     
  }

  template <class T> T readBlock(const word address){	
    if ((address+sizeof(T)-1) >= getCapacity()) {
      return T();
    }	
    T value;
    read_block(address, &value, sizeof(value));     
    return value;
  }

  template <class T> word writeBlock(word address, const T value[], word items){	
    if ((address+items*sizeof(T)-1) >= getCapacity()) {
      return 0;
    }
    word rez;
    for (word i = 0; i < items; i++) {
      rez += writeBlock<T>(address+(i*sizeof(T)),value[i]);
    }
    return rez;
  }

  template <class T> word writeBlock(const word address, const T& value){
    if ((address+sizeof(value)-1) >= getCapacity()) {
      return 0;
    }
    write_block(address, &value, sizeof(value));		  			  
    return sizeof(value);
  }

  template <class T> word updateBlock(const word address, const T value[], word items) {
    
    if ((address+items*sizeof(T)-1) >= getCapacity()) {
      return 0;
    }
    word rez=0;
    for (word i = 0; i < items; i++){
      rez+= updateBlock<T>(address+i*sizeof(T),value[i]);
    }
    return rez;
  }

  template <class T> int updateBlock(word address, const T& value) {
    word writeCount=0;
    if ((address+sizeof(value)-1)  >= getCapacity()) {
      return 0;
    }
    const byte* bytePointer = (const byte*)(const void*)&value;
    for (word i = 0; i < sizeof(value); i++) {
      if (read_byte(address) != *bytePointer) {
        write_byte(address, *bytePointer);
        writeCount++;		
      }
      address++;
      bytePointer++;
    }
    return writeCount;
  }  
  
private:
  virtual void write_byte(const word address, const byte data) ;
  virtual byte read_byte(word address);
  virtual void write_block(const word address, const void* data, const word sizeofData);
  virtual void read_block(const word address, void* data, const word sizeofData);

};

extern EEPROM_ARDUINO_Class EEPROM;

#endif





