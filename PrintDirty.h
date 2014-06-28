#ifndef GB_PrintDirty_h
#define GB_PrintDirty_h

class GB_PrintDirty {

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

  static void printHEX(byte digits){
    // utility function for digital clock display: prints preceding colon and leading 0
    if(digits < 0x10 )
      Serial.print('0');
    Serial.print(digits, HEX);
  }


  static void printHEX(const String &input){   
    for(int i = 0; i<input.length(); i++){
      byte c = input[i];
      printHEX(c);
      if ((i+1)<input.length()) {
        Serial.print(' '); 
      }
    }
  }  
  
  static void printWithoutCRLF(const String &input){   
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
   
  static void printRAM(void *ptr, byte sizeOf){
    byte* buffer =(byte*)ptr;
    for(byte i=0; i<sizeOf; i++){
      printHEX(buffer[i]);
      Serial.print(' ');
    }
  }

};
#endif








