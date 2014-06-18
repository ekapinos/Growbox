#include <MemoryFree.h>;

#include "SerialMonitor.h"

#include "Controller.h"
#include "Logger.h"

void printEnd(){
  if (g_UseSerialWifi) {
    delay(250);
    while (Serial.available()){
      Serial.read();
    }
  }
}

void printLogRecord(word address, word index, boolean events, boolean errors, boolean temperature){
  LogRecord logRecord(0);
  STORAGE.read(address, &logRecord, sizeof(LogRecord));

  if (LOGGER.isEvent(logRecord) && !events){
    return;
  }
  if (LOGGER.isError(logRecord) && !errors){
    return;
  }
  if (LOGGER.isTemperature(logRecord) && !temperature){
    return;
  }

  Serial.print('#');
  Serial.print(index);
  Serial.print(' ');
  LOGGER.printLogRecord(logRecord);
}

void printFullLog(boolean events, boolean errors, boolean temperature){
  word index = 1;
  if (BOOT_RECORD.boolPreferencies.isLogOverflow) {
    for (word i = BOOT_RECORD.nextLogRecordAddress; i < (STORAGE.capacity-sizeof(LogRecord)) ; i+=sizeof(LogRecord)){
      printLogRecord(i, index++, events,  errors,  temperature);
    }
  }
  for (word i = sizeof(BootRecord); i < BOOT_RECORD.nextLogRecordAddress ; i+=sizeof(LogRecord)){
    printLogRecord(i, index++,  events,  errors,  temperature);
  }
  if (index == 1){
    Serial.println(F("- no records in log"));
  }
}


void printStatus(){
  printFreeMemory();
  printBootStatus();
  printTimeStatus();
  printTemperatureStatus();
  printPinsStatus();
  Serial.println();
}

void printFreeMemory(){
  Serial.print(F("Free memory: "));     // print how much RAM is available.
  Serial.print(freeMemory(), DEC);  // print how much RAM is available.
  Serial.println(F(" bytes"));     // print how much RAM is available.
}

void printBootStatus(){
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
  word slotsCount = (STORAGE.capacity - sizeof(BootRecord))/sizeof(LogRecord) ;
  if (BOOT_RECORD.boolPreferencies.isLogOverflow){
    Serial.print(slotsCount);
    Serial.print('/');
    Serial.print(slotsCount);
    Serial.print(F(", overflow"));
  } 
  else {
    Serial.print((BOOT_RECORD.nextLogRecordAddress - sizeof(BootRecord))/sizeof(LogRecord));
    Serial.print('/');
    Serial.print((STORAGE.capacity - sizeof(BootRecord))/sizeof(LogRecord));
  }
  Serial.println();
}

void printTimeStatus(){
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

void printTemperatureStatus(){
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
void printPinsStatus(){
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

void printTime(time_t time){
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

void print2digits(byte digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void print2digitsHEX(byte digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 0x10 )
    Serial.print('0');
  Serial.print(digits, HEX);
}

void printRAM(void *ptr, byte sizeOf){
  byte* buffer =(byte*)ptr;
  for(byte i=0; i<sizeOf; i++){
    print2digitsHEX(buffer[i]);
    Serial.print(' ');
  }
  Serial.println();
}

void printStorage(word address, byte sizeOf){
  byte buffer[sizeOf];
  STORAGE.read(address, buffer, sizeOf);
  printRAM(buffer, sizeOf);
}

void printStorage(){

  Serial.print(F("  "));
  for (word i = 0; i < 16 ; i++){
    Serial.print(F("  "));
    Serial.print(i, HEX); 
    Serial.print(' ');
  }

  //long errors = 0;
  for (word i = 0; i < STORAGE.capacity ; i++){
    byte value = STORAGE.read(i);

    if (i% 16 ==0){
      Serial.println();
      print2digitsHEX(i/16);
    }
    Serial.print(" ");
    print2digitsHEX(value);
    Serial.print(" ");
    //    if (value != setvalue){
    //      errors++;
    //    }
  }

  Serial.println();  
  //  if (setvalues){
  //    Serial.print("Set to 0x");
  //    print2digitsHEX(setvalue) ; 
  //    Serial.print(" errors: ");
  //    Serial.println(errors); 
  //  }
  // Serial.println(); 
}

void fillStorage(byte value){

  if (value != 'i'){
    Serial.println(F("Filling STORAGE with value: "));
    print2digitsHEX(value);
    Serial.println();
    for (word i = 0; i < STORAGE.capacity ; i++){
      STORAGE.write(i, value);
    }

  } 
  else {
    Serial.println(F("Filling STORAGE with incremental values "));
    for (word i = 0; i < STORAGE.capacity ; i++){
      STORAGE.write(i, (byte)i);
    }  
  }
}

void checkSerialCommands(){

  if (!g_UseSerialMonitor){
    return;
  } 

  // send data only when you receive data:
  if (Serial.available() > 0) {

    // read the incoming byte:
    char firstChar = Serial.read();

    char secondChar;
    if (Serial.available() > 0) {
      secondChar = Serial.read();
    } 
    else {
      secondChar = 0;
    }

    Serial.print(F("GB>"));
    Serial.print(firstChar);
    if (secondChar != 0){
      Serial.print(secondChar);
    }
    Serial.println();

    boolean events = true; // can't put in switch, Arduino bug
    boolean errors = true;
    boolean temperature = true;

    switch(firstChar){
    case 's':
      printStatus(); 
      break;  

    case 'l':
      switch(secondChar){
      case 'c': 
        Serial.println(F("Cleaning log records"));
        BOOT_RECORD.cleanupLog();
        break;
      case 'e':
        Serial.println(F("Logger enabled"));
        BOOT_RECORD.setLoggerEnable(true);
        break;
      case 'd':
        Serial.println(F("Logger disabled"));
        BOOT_RECORD.setLoggerEnable(false);
        break;
      case 't': 
        errors = events = false;
        break;
      case 'r': 
        events = temperature = false;
        break;
      case 'v': 
        errors = temperature = false;
        break;
      } 
      if ((secondChar != 'c') && (secondChar != 'e') && (secondChar != 'd')){
        printFullLog(events ,  errors ,  temperature );
      }
      break; 

    case 'b': 
      switch(secondChar){
      case 'c': 
        Serial.println(F("Cleaning boot record"));

        BOOT_RECORD.first_magic = 0;
        STORAGE.write(0, &BOOT_RECORD, sizeof(BootRecord));
        Serial.println(F("Magic number corrupted, reseting"));

        Serial.println('5');
        delay(1000);
        Serial.println('4');
        delay(1000);
        Serial.println('3');
        delay(1000);
        Serial.println('2');
        delay(1000);
        Serial.println('1');
        delay(1000);
        rebootController();
        break;
      } 

      Serial.println(F("Currnet boot record"));
      Serial.print(F("-Memory : ")); 
      printRAM(&BOOT_RECORD, sizeof(BootRecord));
      Serial.print(F("-Storage: ")); 
      printStorage(0, sizeof(BootRecord));

      break;  

    case 'm':    
      switch(secondChar){
      case '0': 
        fillStorage(0x00); 
        break; 
      case 'a': 
        fillStorage(0xAA); 
        break; 
      case 'f': 
        fillStorage(0xFF); 
        break; 
      case 'i': 
        fillStorage('i'); 
        break; 
      }
      printStorage();
      break; 
    case 'r':        
      rebootController();
      break; 
    default: 
      LOGGER.logEvent(EVENT_SERIAL_UNKNOWN_COMMAND);  
    }
    delay(1000);               // wait for a second
  }
}




