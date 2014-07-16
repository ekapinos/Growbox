#include "EEPROM_AT24C32.h"

boolean EEPROM_AT24C32_Class::isPresent(void) {    
  Wire.beginTransmission(I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
    return true;
  return false;
}

void EEPROM_AT24C32_Class::write(const word address, const byte data) {
  if (!isWriteOk(address)){
    return;
  }
  Wire.beginTransmission(I2C_ADDRESS);
  Wire.write((byte)(address >> 8)); // MSB
  Wire.write((byte)(address & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();  
  delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
}

byte EEPROM_AT24C32_Class::read(word address) {
  if (!isWriteOk(address)){
    return 0xFF;
  }
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


/////////////////////////////////////////////////////////////////////
//                          EXTRA COMMANDS                         //
/////////////////////////////////////////////////////////////////////

void EEPROM_AT24C32_Class::fillStorage(byte value){
  for (word i = 0; i < CAPACITY ; i++){
    write(i, value);
  }
}

void EEPROM_AT24C32_Class::fillStorageIncremental(){      
  for (word i = 0; i < CAPACITY ; i++){
    write(i, (byte)i);
  }  
}

// private 

boolean EEPROM_AT24C32_Class::isWriteOk(const word address){
  return (address < CAPACITY);
}
void EEPROM_AT24C32_Class::write_block(const word address, const void* data, const byte sizeofData) {
  // We have problems with sequental read/write (internal counter has only 5 bits, see spec)
  for (word c = 0; c < sizeofData; c++){
    byte value = ((byte*)data)[c];
    write(address + c, value);  
  }
}

void EEPROM_AT24C32_Class::read_block(const word address, void *data, const byte sizeofData) {
  // We have problems with sequental read/write (internal counter has only 5 bits, see spec)
  for (word c = 0; c < sizeofData; c++){
    byte value =  read(address + c);
    ((byte*)data)[c] = value;
  }
}

EEPROM_AT24C32_Class EEPROM_AT24C32;


