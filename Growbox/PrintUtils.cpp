#include "PrintUtils.h"

void PrintUtils::printHEX(const String &input){   
  for(int i = 0; i<input.length(); i++){
    byte c = input[i];
    Serial.print(StringUtils::getHEX(c, 2));
    if ((i+1)<input.length()) {
      Serial.print(' '); 
    }
  }
}  

void PrintUtils::printWithoutCRLF(const String &input){   
  for (int i = 0; i<input.length(); i++){
    if (input[i] == '\r'){
      Serial.print(F("\\r"));
    } 
    else if (input[i] == '\n'){
      Serial.print(F("\\n"));
    } 
    else {
      Serial.print(input[i]);
    }
  }
}

void PrintUtils::printRAM(void *ptr, byte sizeOf){
  byte* buffer =(byte*)ptr;
  for(byte i=0; i<sizeOf; i++){
    Serial.print(StringUtils::getHEX(buffer[i], 2));
    Serial.print(' ');
  }
}

void PrintUtils::printFreeMemory(){  
  Serial.print(FS(S_Free_memory));
  Serial.print(freeMemory()); 
  Serial.println(FS(S_bytes));
}












