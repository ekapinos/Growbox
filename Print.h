#ifndef GB_Print_h
#define GB_Print_h

#include <Time.h>
#include <MemoryFree.h>;

#include "BootRecord.h";

class GB_Print {

public:
  static void printEnd(){
    if (g_UseSerialWifi) {
      delay(250);
      while (Serial.available()){
        Serial.read();
      }
    }
  }

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

  static void printStatus(){
    printFreeMemory();
    printBootStatus();
    printTimeStatus();
    printTemperatureStatus();
    printPinsStatus();
    Serial.println();
  }
  
  
  
 private:
 
  static void printBootStatus(){
    Serial.print(F("Controller: frist startup time: ")); 
    printTime(BOOT_RECORD.firstStartupTimeStamp);
    Serial.print(F(", last startup time: ")); 
    printTime(BOOT_RECORD.lastStartupTimeStamp);
    Serial.println();
    Serial.print(F("Logger: "));
    if (BOOT_RECORD.boolPreferencies.isLoggerEnabled){
      Serial.print(F("enabled"));
    } 
    else {
      Serial.print(F("disabled"));
    }
    Serial.print(F(", records: "));
    word slotsCount = (GB_Storage::CAPACITY - sizeof(BootRecord))/sizeof(LogRecord) ;
    if (BOOT_RECORD.boolPreferencies.isLogOverflow){
      Serial.print(slotsCount);
      Serial.print('/');
      Serial.print(slotsCount);
      Serial.print(F(", overflow"));
    } 
    else {
      Serial.print((BOOT_RECORD.nextLogRecordAddress - sizeof(BootRecord))/sizeof(LogRecord));
      Serial.print('/');
      Serial.print((GB_Storage::CAPACITY - sizeof(BootRecord))/sizeof(LogRecord));
    }
    Serial.println();
  }

  static void printTimeStatus(){
    Serial.print(F("Clock: mode:")); 
    if (g_isDayInGrowbox) {
      Serial.print(F("DAY"));
    } 
    else{
      Serial.print(F("NIGHT"));
    }
    Serial.print(F(", current time: ")); 
    printTime(now());
    Serial.print(F(", up time: [")); 
    Serial.print(UP_HOUR); 
    Serial.print(F(":00], down time: ["));
    Serial.print(DOWN_HOUR);
    Serial.print(F(":00], "));
    Serial.println();
  }

  static void printTemperatureStatus(){
    Serial.print(F("Temperature: current:")); 
    Serial.print(g_temperature);

    float statisticsTemperature;
    if (g_temperatureSummCount != 0){
      statisticsTemperature = g_temperatureSumm/g_temperatureSummCount;
    } 
    else {
      statisticsTemperature = g_temperature;
    }
    Serial.print(F(", count:")); 
    Serial.print(statisticsTemperature);
    Serial.print(F("(el:")); 
    Serial.print(g_temperatureSummCount);

    Serial.print(F("), day:"));
    Serial.print(TEMPERATURE_DAY);
    Serial.print(F("+/-"));
    Serial.print(TEMPERATURE_DELTA);
    Serial.print(F(", night:"));
    Serial.print(TEMPERATURE_NIGHT);
    Serial.print(F("+/-"));
    Serial.print(2*TEMPERATURE_DELTA);
    Serial.print(F(", critical:"));
    Serial.print(TEMPERATURE_CRITICAL);
    Serial.println();
  }

  static void printPinsStatus(){
    Serial.println();
    Serial.println(F("Pin OUTPUT INPUT")); 
    for(int i=0; i<=19;i++){
      Serial.print(' ');
      if (i>=14){
        Serial.print('A');
        Serial.print(i-14);
      } 
      else { 
        print2digits(i);
      }
      Serial.print(F("  ")); 

      boolean io_status, dataStatus, inputStatus;
      if (i<=7){ 
        io_status = bitRead(DDRD, i);
        dataStatus = bitRead(PORTD, i);
        inputStatus = bitRead(PIND, i);
      }    
      else if (i <= 13){
        io_status = bitRead(DDRB, i-8);
        dataStatus = bitRead(PORTB, i-8);
        inputStatus = bitRead(PINB, i-8);
      }
      else {
        io_status = bitRead(DDRC, i-14);
        dataStatus = bitRead(PORTC, i-14);
        inputStatus = bitRead(PINC, i-14);
      }
      if (io_status == OUTPUT){
        Serial.print(F("  "));
        Serial.print(dataStatus);
        Serial.print(F("     -   "));
      } 
      else {
        Serial.print(F("  -     "));
        Serial.print(inputStatus);
        Serial.print(F("   "));
      }

      switch(i){
      case 0: 
      case 1: 
        Serial.print(F("Reserved by Serial/USB. Can be used, if Serial/USB won't be connected"));
        break;
      case LIGHT_PIN: 
        Serial.print(F("Relay: light on(0)/off(1)"));
        break;
      case FAN_PIN: 
        Serial.print(F("Relay: fun on(0)/off(1)"));
        break;
      case FAN_SPEED_PIN: 
        Serial.print(F("Relay: fun max(0)/min(1) speed switch"));
        break;
      case ONE_WIRE_PIN: 
        Serial.print(F("1-Wire: termometer"));
        break;
      case ERROR_PIN: 
        Serial.print(F("Error status"));
        break;
      case BREEZE_PIN: 
        Serial.print(F("Breeze"));
        break;
      case 18: 
      case 19: 
        Serial.print(F("Reserved by I2C. Can be used, if SCL, SDA pins will be used"));
        break;
      }
      Serial.println();
    }
  }

};
#endif






