#ifndef GB_Storage_h
#define GB_Storage_h

#include <Wire.h> 
#include "Global.h" 

class StorageClass {
private:
  static const int AT24C32 = 0x50; // External EEPROM I2C address
public:
  static const word capacity = 0x1000; // 4K byte = 32K bit

  boolean isPresent(void) {     // check if the device is present
    Wire.beginTransmission(AT24C32);
    if (Wire.endTransmission() == 0)
      return true;
    return false;
  }

  void write(const word address, const byte data) {
    if (address >= capacity){
      return;
    }
    Wire.beginTransmission(AT24C32);
    Wire.write((byte)(address >> 8)); // MSB
    Wire.write((byte)(address & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();  
    delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
  }

  byte read(word address) {
    Wire.beginTransmission(AT24C32);
    Wire.write((byte)(address >> 8)); // MSB
    Wire.write((byte)(address & 0xFF)); // LSB
    Wire.endTransmission();
    delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
    Wire.requestFrom(AT24C32, 1);
    if (Wire.available()) {
      return Wire.read();
    } 
    else {
      return 0xFF;
    }
  }

  void write(word address, const void* data, const byte sizeofData) {
    for (word c = 0; c < sizeofData; c++){
      byte value = ((byte*)data)[c];
      write(address + c, value);
    }
  }

  void read(word address, void *data, const byte sizeofData) {
    for (word c = 0; c < sizeofData; c++){
      byte value =  read(address + c);
      ((byte*)data)[c] = value;
    }
  }
}; 

extern StorageClass STORAGE;

#endif


