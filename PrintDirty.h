#ifndef GB_PrintDirty_h
#define GB_PrintDirty_h

class GB_PrintDirty {
public:

  static String getFixedDigitsString(const int number, const byte numberOfDigits){
    String out;
    for (int i = 0; i< numberOfDigits; i++){
      out +='0';
    }
    out += number;
    return out.substring(out.length()-numberOfDigits);
  }

  static String getHEX(byte number, boolean addPrefix = false){
    String out(number, HEX);
    if(number < 0x10){
      out = String('0') + out;
    }
    if (addPrefix){
      out = String("0x") + out;
    }
    return out;
  }
  static String floatToString(float number){
    String out;

    int temp = number*100;
    int whole = temp/100;
    int fract = temp%100;

    out += whole;
    out += '.';
    out += getFixedDigitsString(temp,2);
    return out;
  }


  static String getTimeString(time_t time){
    String out;

    tmElements_t tm;
    breakTime(time, tm);

    out += '[';
    out += getFixedDigitsString(tm.Hour, 2);
    out += ':';
    out += getFixedDigitsString(tm.Minute, 2);
    out += ':';
    out += getFixedDigitsString(tm.Second, 2);
    out += ' ';
    out += getFixedDigitsString(tm.Day, 2);
    out +='.';
    out += getFixedDigitsString(tm.Month, 2);
    out += '.';
    out += getFixedDigitsString(tmYearToCalendar(tm.Year), 4); 
    out += ']';
    return out;
  } 


  // utility function for digital clock display: prints preceding colon and leading 0
  static void print2digits(byte number){
    Serial.print(getFixedDigitsString(number, 2));
  }

  static void printHEX(byte number, boolean addPrefix = false){
    Serial.print(getHEX(number));
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










