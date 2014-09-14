#include "EEPROM_AT24C32.h"

#include <Wire.h>

#define AT24C32_I2C_ADDRESS   0x50 // External EEPROM I2C address
#define AT24C32_CAPACITY      0x1000

word EEPROM_AT24C32_Class::getCapacity(){
    return AT24C32_CAPACITY;  // 4K byte = 32K bit
}

boolean EEPROM_AT24C32_Class::isPresent() {    
  Wire.beginTransmission(AT24C32_I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

void EEPROM_AT24C32_Class::write_byte(const word address, const byte data) {
  Wire.beginTransmission(AT24C32_I2C_ADDRESS);
  Wire.write((byte)(address >> 8)); // MSB
  Wire.write((byte)(address & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();  
  delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
}

byte EEPROM_AT24C32_Class::read_byte(word address) {
  Wire.beginTransmission(AT24C32_I2C_ADDRESS);
  Wire.write((byte)(address >> 8)); // MSB
  Wire.write((byte)(address & 0xFF)); // LSB
  Wire.endTransmission();
  delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
  Wire.requestFrom(AT24C32_I2C_ADDRESS, 1);
  if (Wire.available()) {
    return Wire.read();
  } 
  else {
    return 0;
  }
}

void EEPROM_AT24C32_Class::write_block(const word address, const void* data, const word sizeofData) {
  // We have problems with sequental read/write (internal counter has only 5 bits, see spec)
  for (word c = 0; c < sizeofData; c++){
    byte value = ((byte*)data)[c];
    write(address + c, value);  
  }
}

void EEPROM_AT24C32_Class::read_block(const word address, void *data, const word sizeofData) {
  // We have problems with sequental read/write (internal counter has only 5 bits, see spec)
  for (word c = 0; c < sizeofData; c++){
    byte value =  read(address + c);
    ((byte*)data)[c] = value;
  }
}

EEPROM_AT24C32_Class EEPROM_AT24C32;


