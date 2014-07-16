#ifndef EEPROMEX_PATCH_h
#define EEPROMEX_PATCH_h

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

//#include <inttypes.h>
#include <avr/eeprom.h>

class EEPROMClassExPatch{

public:
  /////////////////////////////////////////////////////////////////////
  //                            TEMPLATES                            //
  /////////////////////////////////////////////////////////////////////


  // Templates copied from: http://thijs.elenbaas.net/2012/07/extended-eeprom-library-for-arduino/
  //
  //  EEPROMEx.h - Extended EEPROM library
  //  Copyright (c) 2012 Thijs Elenbaas.  All right reserved.

  // Use template for other data formats


  template <class T> int readBlock(int address, const T value[], int items)
  {
    if (!isWriteOk(address+items*sizeof(T))) return 0;
    unsigned int i;
    for (i = 0; i < (unsigned int)items; i++)
      readBlock<T>(address+(i*sizeof(T)),value[i]);
    return i;
  }

  template <class T> int readBlock(int address, const T& value)
  {		
    eeprom_read_block((void*)&value, (const void*)address, sizeof(value));
    return sizeof(value);
  }

  template <class T> int writeBlock(int address, const T value[], int items)
  {	
    if (!isWriteOk(address+items*sizeof(T))) return 0;
    unsigned int i;
    for (i = 0; i < (unsigned int)items; i++)
      writeBlock<T>(address+(i*sizeof(T)),value[i]);
    return i;
  }

  template <class T> int writeBlock(int address, const T& value)
  {
    if (!isWriteOk(address+sizeof(value))) return 0;
    eeprom_write_block((void*)&value, (void*)address, sizeof(value));			  			  
    return sizeof(value);
  }

  template <class T> int updateBlock(int address, const T value[], int items)
  {
    int writeCount=0;
    if (!isWriteOk(address+items*sizeof(T))) return 0;
    unsigned int i;
    for (i = 0; i < (unsigned int)items; i++)
      writeCount+= updateBlock<T>(address+(i*sizeof(T)),value[i]);
    return writeCount;
  }

  template <class T> int updateBlock(int address, const T& value)
  {
    int writeCount=0;
    if (!isWriteOk(address+sizeof(value))) return 0;
    const byte* bytePointer = (const byte*)(const void*)&value;
    for (unsigned int i = 0; i < (unsigned int)sizeof(value); i++) {
      if (eeprom_read_byte((unsigned char *) address)!=*bytePointer) {
        eeprom_write_byte((unsigned char *) address, *bytePointer);
        //Serial.print(F("STORAGE-INTRNAL> write address: "));Serial.print(address); Serial.print(F(" value: "));Serial.println(*bytePointer);
        writeCount++;		
      }
      address++;
      bytePointer++;
    }
    return writeCount;
  }

  boolean isWriteOk(const word address);
  
  boolean isReady();
  
};

extern EEPROMClassExPatch EEPROM;

#endif


