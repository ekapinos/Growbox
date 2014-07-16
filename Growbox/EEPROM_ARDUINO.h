#ifndef EEPROM_ARDUINO_h
#define EEPROM_ARDUINO_h

// Copied and adapted from: https://github.com/thijse/Arduino-Libraries/blob/master/EEPROMEx
//
//  EEPROMEx.h - Extended EEPROM library
//  Copyright (c) 2012 Thijs Elenbaas.  All right reserved.

#if ARDUINO >= 100
#include <Arduino.h> 
#else
#include <WProgram.h> 
#endif

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

#ifndef EEPROM_CAPACITY
// Define your device before import
#define EEPROM_CAPACITY       EEPROMSizeMega
#endif

class EEPROM_ARDUINO_Class{

  static const word CAPACITY = EEPROM_CAPACITY;

public:

  virtual boolean isWriteOk(const word address); 
  virtual boolean isReady();

  virtual void write(const word address, const byte data) ;
  virtual byte read(word address);
  virtual void write_block(const word address, const void* data, const word sizeofData);
  virtual void read_block(const word address, void *data, const word sizeofData);


  /////////////////////////////////////////////////////////////////////
  //                            TEMPLATES                            //
  /////////////////////////////////////////////////////////////////////

  template <class T> word readBlock(word address, const T value[], word items){
    if (!isWriteOk(address+items*sizeof(T)-1)) {
      return 0;
    }
    word i;
    for (i = 0; i < items; i++) {
      readBlock<T>(address+(i*sizeof(T)),value[i]); 
    }     
    return i;
  }

  template <class T> word readBlock(word address, T& value){	
    if (!isWriteOk(address+sizeof(value)-1)) {
      return 0;
    }	
    read_block(address, &value, sizeof(value));     
    return sizeof(value);
  }

  template <class T> word writeBlock(word address, const T value[], word items){	
    if (!isWriteOk(address+items*sizeof(T)-1)) {
      return 0;
    }
    word i;
    for (i = 0; i < items; i++) {
      writeBlock<T>(address+(i*sizeof(T)),value[i]);
    }
    return i;
  }

  template <class T> word writeBlock(word address, const T& value){
    if (!isWriteOk(address+sizeof(value)-1)) {
      return 0;
    }
    write_block(address, &value, sizeof(value));		  			  
    return sizeof(value);
  }

  template <class T> word updateBlock(word address, const T value[], word items) {
    int writeCount=0;
    if (!isWriteOk(address+items*sizeof(T)-1)) {
      return 0;
    }
    for (word i = 0; i < items; i++){
      writeCount+= updateBlock<T>(address+(i*sizeof(T)),value[i]);
    }
    return writeCount;
  }

  template <class T> int updateBlock(word address, const T& value) {
    word writeCount=0;
    if (!isWriteOk(address+sizeof(value)-1)) {
      return 0;
    }
    const byte* bytePointer = (const byte*)(const void*)&value;
    for (word i = 0; i < sizeof(value); i++) {
      if (read(address) != *bytePointer) {
        write(address, *bytePointer);
        //Serial.print(F("STORAGE-INTRNAL> write address: "));Serial.print(address); Serial.print(F(" value: "));Serial.println(*bytePointer);
        writeCount++;		
      }
      address++;
      bytePointer++;
    }
    return writeCount;
  }  
};

extern EEPROM_ARDUINO_Class EEPROM;

#endif





