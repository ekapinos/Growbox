#ifndef GB_Print_h
#define GB_Print_h

#include <MemoryFree.h>

class GB_Print {

public:

  static void printTime(time_t time){
    tmElements_t tm;
    breakTime(time, tm);

    Serial.print('[');
    // digital clock display of the time
    print2digits(tm.Hour);
    Serial.print(':');
    print2digits(tm.Minute);
    Serial.print(':');
    print2digits(tm.Second);
    Serial.print(' ');
    print2digits(tm.Day);
    Serial.print('.');
    print2digits(tm.Month);
    Serial.print('.');
    Serial.print(tmYearToCalendar(tm.Year)); 
    Serial.print(']');
  }

  static void print2digits(byte digits){
    // utility function for digital clock display: prints preceding colon and leading 0
    if(digits < 10)
      Serial.print('0');
    Serial.print(digits);
  }

  static void print2digitsHEX(byte digits){
    // utility function for digital clock display: prints preceding colon and leading 0
    if(digits < 0x10 )
      Serial.print('0');
    Serial.print(digits, HEX);
  }
  
  static void printFreeMemory(){
    Serial.print(F("Free memory: "));     // print how much RAM is available.
    Serial.print(freeMemory(), DEC);  // print how much RAM is available.
    Serial.println(F(" bytes"));     // print how much RAM is available.
  }

  static void printRAM(void *ptr, byte sizeOf){
    byte* buffer =(byte*)ptr;
    for(byte i=0; i<sizeOf; i++){
      print2digitsHEX(buffer[i]);
      Serial.print(' ');
    }
    Serial.println();
  }

  static void printStorage(word address, byte sizeOf){
    byte buffer[sizeOf];
    GB_Storage::read(address, buffer, sizeOf);
    printRAM(buffer, sizeOf);
  }

  static void printStorage(){

    Serial.print(F("  "));
    for (word i = 0; i < 16 ; i++){
      Serial.print(F("  "));
      Serial.print(i, HEX); 
      Serial.print(' ');
    }

    for (word i = 0; i < GB_Storage::CAPACITY ; i++){
      byte value = GB_Storage::read(i);

      if (i% 16 ==0){
        Serial.println();
        print2digitsHEX(i/16);
      }
      Serial.print(" ");
      print2digitsHEX(value);
      Serial.print(" ");
    }

    Serial.println();  

  }
};
#endif






