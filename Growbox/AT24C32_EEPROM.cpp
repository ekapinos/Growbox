#include "AT24C32_EEPROM.h"

boolean AT24C32_EEPROM_Class::isPresent(void) {    
  Wire.beginTransmission(I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

void AT24C32_EEPROM_Class::write(const word address, const byte data) {
  if (address >= CAPACITY){
    return;
  }
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write((byte)(address >> 8)); // MSB
  Wire.write((byte)(address & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();  
  delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
}

byte AT24C32_EEPROM_Class::read(word address) {
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write((byte)(address >> 8)); // MSB
  Wire.write((byte)(address & 0xFF)); // LSB
  Wire.endTransmission();
  delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
  Wire.requestFrom(I2C_ADDRESS, 1);
  if (Wire.available()) {
    return Wire.read();
  } 
  else {
    return 0xFF;
  }
}

void AT24C32_EEPROM_Class::write(word address, const void* data, const byte sizeofData) {
  // We have problems with sequental read/write (internal counter has only 5 bits, see spec)
  for (word c = 0; c < sizeofData; c++){
    byte value = ((byte*)data)[c];
    write(address + c, value);  
  }
}

void AT24C32_EEPROM_Class::read(word address, void *data, const byte sizeofData) {
  // We have problems with sequental read/write (internal counter has only 5 bits, see spec)
  for (word c = 0; c < sizeofData; c++){
    byte value =  read(address + c);
    ((byte*)data)[c] = value;
  }
}

/////////////////////////////////////////////////////////////////////
//                          EXTRA COMMANDS                         //
/////////////////////////////////////////////////////////////////////

void AT24C32_EEPROM_Class::fillStorage(byte value){
  for (word i = 0; i < CAPACITY ; i++){
    write(i, value);
  }
}

void AT24C32_EEPROM_Class::fillStorageIncremental(){      
  for (word i = 0; i < CAPACITY ; i++){
    write(i, (byte)i);
  }  
}

AT24C32_EEPROM_Class AT24C32_EEPROM;


