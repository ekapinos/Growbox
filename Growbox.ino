
// Warning! We need to include all used libraries, 
// otherwise Arduino IDE doesn't set correct build 
// params for gcc compilator
#include <MemoryFree.h>

#include <Time.h>
#include <TimeAlarms.h>

// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

// Termometer
#include <OneWire.h>
#include <DallasTemperature.h>

// Growbox
#include "Global.h"
#include "Controller.h"
#include "Thermometer.h"
#include "SerialHelper.h"

#define GB_EXTRA_STRINGS

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

byte g_isDayInGrowbox = -1;

/////////////////////////////////////////////////////////////////////
//                              STATUS                             //
/////////////////////////////////////////////////////////////////////

boolean isDayInGrowbox(){
  if(timeStatus() == timeNeedsSync){
    GB_Logger::logError(ERROR_TIMER_NEEDS_SYNC);
  } 
  else {
    GB_Logger::stopLogError(ERROR_TIMER_NEEDS_SYNC);
  }

  int currentHour = hour();

  boolean isDayInGrowbox = false;
  if (UP_HOUR < DOWN_HOUR){
    if (UP_HOUR < currentHour && currentHour < DOWN_HOUR ){
      isDayInGrowbox = true;
    }
  } 
  else { // UP_HOUR > DOWN_HOUR
    if (UP_HOUR < currentHour || currentHour < DOWN_HOUR ){
      isDayInGrowbox = true;
    }
  } 
  return isDayInGrowbox;
}


/////////////////////////////////////////////////////////////////////
//                                MAIN                             //
/////////////////////////////////////////////////////////////////////


// the setup routine runs once when you press reset:
void setup() {                

  // Initialize the info pins
  pinMode(LED_BUILTIN, OUTPUT); // brease
  pinMode(BREEZE_PIN, OUTPUT);  // brease
  pinMode(ERROR_PIN, OUTPUT);
  pinMode(USE_SERIAL_MONOTOR_PIN, INPUT_PULLUP);

  // Initialize the reley pins
  pinMode(LIGHT_PIN, OUTPUT);   
  pinMode(FAN_PIN, OUTPUT);  
  pinMode(FAN_SPEED_PIN, OUTPUT); 

  // Configure relay
  digitalWrite(LIGHT_PIN, RELAY_OFF);
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);

  // Configure inputs
  //attachInterrupt(0, interrapton0handler, CHANGE); // PIN 2

  // We need to check Wi-Fi before use print to SerialMonitor
  g_isGrowboxStarted = false;

  GB_SerialHelper::checkSerial(true, true);

  // We should init Errors & Events before checkSerialWifi->(), cause we may use them after
  if(GB_SerialHelper::useSerialMonitor){ 
    printFreeMemory();
    Serial.println(F("Checking software configuration..."));
    GB_SerialHelper::printDirtyEnd();
  }

  initLoggerModel();
  if (!Error::isInitialized()){
    if(GB_SerialHelper::useSerialMonitor){ 
      Serial.print(F("Fatal error: not all Errors initialized"));
      GB_SerialHelper::printDirtyEnd();
    }
    while(true) delay(5000);  
  }
  if (!Event::isInitialized()){
    if(GB_SerialHelper::GB_SerialHelper::useSerialMonitor){ 
      Serial.print(F("Fatal error: not all Events initialized"));
      GB_SerialHelper::printDirtyEnd();
    }
    while(true) delay(5000);  
  }

  if (BOOT_RECORD_SIZE != sizeof(BootRecord)){
    digitalWrite(ERROR_PIN, HIGH);
    if(GB_SerialHelper::useSerialMonitor){ 
      Serial.print(F("Fatal error: wrong BootRecord size. Exepted:"));
      Serial.print(BOOT_RECORD_SIZE);
      Serial.print(F(", current: "));
      Serial.print(sizeof(BootRecord));
      GB_SerialHelper::printDirtyEnd();
    }
    while(true) delay(5000);
  }

  if (LOG_RECORD_SIZE != sizeof(LogRecord)){
    digitalWrite(ERROR_PIN, HIGH);
    if(GB_SerialHelper::useSerialMonitor){ 
      Serial.print(F("Fatal error: wrong LogRecord size. Exepted:"));
      Serial.print(BOOT_RECORD_SIZE);
      Serial.print(F(", current: "));
      Serial.print(sizeof(BootRecord));
      GB_SerialHelper::printDirtyEnd();
    }
    while(true) delay(5000);
  }

  GB_Controller::checkFreeMemory();

  if(GB_SerialHelper::useSerialMonitor){ 
    Serial.println(F("Checking clock..."));
    GB_SerialHelper::printDirtyEnd();
  }

  // Configure clock
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  while(timeStatus() == timeNotSet || 2014<year() || year()>2020) { // ... and check it
    GB_Logger::logError(ERROR_TIMER_NOT_SET);
    setSyncProvider(RTC.get); // try to refresh clock
  }
  GB_Logger::stopLogError(ERROR_TIMER_NOT_SET); 

  GB_Controller::checkFreeMemory();

  if(GB_SerialHelper::useSerialMonitor){ 
    Serial.println(F("Checking termometer..."));
    GB_SerialHelper::printDirtyEnd();
  }

  // Configure termometer
  GB_Thermometer::start();
  while(!GB_Thermometer::updateStatistics()) { // Load temperature on startup
    delay(1000);
  }

  GB_Controller::checkFreeMemory();

  if(GB_SerialHelper::useSerialMonitor){ 
    Serial.println(F("Checking storage..."));
    GB_SerialHelper::printDirtyEnd();
  }

  // Check EEPROM, if Arduino doesn't reboot - all OK
  boolean itWasRestart = GB_StorageHelper::start();

  g_isGrowboxStarted = true;

  // Now we can use logger
  if (itWasRestart){
    GB_Logger::logEvent(EVENT_RESTART);
  } 
  else {
    GB_Logger::logEvent(EVENT_FIRST_START_UP);
  }

  // Log current temeperature
  GB_Thermometer::getTemperature(); // forceLog?

  // Init/Restore growbox state
  if (isDayInGrowbox()){
    switchToDayMode();
  } 
  else {
    switchToNightMode();
  }

  if (GB_SerialHelper::useSerialWifi){    
    GB_SerialHelper::startWifi();
  }

  // Create main life circle timer
  Alarm.timerRepeat(UPDATE_THEMPERATURE_STATISTICS_DELAY, updateThermometerStatistics);  // repeat every N seconds
  Alarm.timerRepeat(CHECK_GROWBOX_DELAY, checkGrowboxState);  // repeat every N seconds

  // Create suolemental rare switching
  Alarm.alarmRepeat(UP_HOUR, 00, 00, switchToDayMode);      // repeat once every day
  Alarm.alarmRepeat(DOWN_HOUR, 00, 00, switchToNightMode);  // repeat once every day

  if(GB_SerialHelper::useSerialMonitor){ 
    Serial.println(F("Growbox successfully started"));
    GB_SerialHelper::printDirtyEnd();
  }

 GB_StorageHelper::setStoreLogRecordsEnabled(true);
  GB_StorageHelper::resetStoredLog();
  for (int i = 0; i<=850; i++){  
    if (i%50 ==0){
      Serial.println(i);
    }  
    GB_Logger::logTemperature(i % 50);

  }
  GB_Logger::printFullLog(true,  true,  true);
  printStorage();
 GB_StorageHelper::setStoreLogRecordsEnabled(false);
  //    LogRecord logRecord;
  //    GB_StorageHelper::getLogRecordByIndex(0, logRecord);
  //    
  //    GB_PrintDirty::printRAM(&logRecord, sizeof(logRecord));




  //   for(int i=0; i<900; i++){
  //     Serial.print(i);
  //     Serial.print(" - ");
  //     GB_PrintDirty::printRAM(&(GB_StorageHelper::bootRecord.nextLogRecordAddress), sizeof(GB_StorageHelper::bootRecord.nextLogRecordAddress));
  //     Serial.println();
  //     GB_StorageHelper::increaseLogPointer();
  //   }

}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(BREEZE_PIN, !digitalRead(BREEZE_PIN));

  GB_Controller::checkFreeMemory();
  GB_SerialHelper::checkSerial(true, false); // not interraption cause Serial print problems

  Alarm.delay(MAIN_LOOP_DELAY * 1000); // wait one second between clock display
}


void serialEvent(){
  if(!g_isGrowboxStarted){
    return; //Do not handle events during startup
  }

  String input; 
  while (Serial.available()){
    input += (char) Serial.read();
  }

  // somthing wrong with Wi-Fi, we need to reboot it
  if (input.indexOf(WIFI_MESSAGE_WELLCOME) >= 0 || input.indexOf(WIFI_MESSAGE_ERROR) >= 0){
    GB_SerialHelper::useSerialWifi = false; // TODO only for logging
    GB_SerialHelper::checkSerial(false, true);
    return;
  }

  input.trim();
  if (input.length() == 0){
    return;
  }
  Serial.print(F("Serial.read: "));
  Serial.println(input);
  GB_SerialHelper::printDirtyEnd();

  //if (g_UseSerialWifi) {
  //
  //} else
  if (GB_SerialHelper::useSerialMonitor) {
    executeCommand(input);
  }
}


/////////////////////////////////////////////////////////////////////
//                  TIMER/CLOCK EVENT HANDLERS                     //
/////////////////////////////////////////////////////////////////////

void checkGrowboxState() {

  float temperature = GB_Thermometer::getTemperature();

  if (temperature >= TEMPERATURE_CRITICAL){
    turnOffLight();
    turnOnFan(FAN_SPEED_MAX);
    GB_Logger::logError(ERROR_TERMOMETER_CRITICAL_VALUE);
  } 
  else if (g_isDayInGrowbox) {
    // Day mode
    turnOnLight();
    if (temperature < (TEMPERATURE_DAY - TEMPERATURE_DELTA)) {
      // Too cold, no heater
      turnOnFan(FAN_SPEED_MIN); // no wind, no grow
    } 
    else if (temperature > (TEMPERATURE_DAY + TEMPERATURE_DELTA)){
      // Too hot
      turnOnFan(FAN_SPEED_MAX);    
    } 
    else {
      // Normal, day wind
      turnOnFan(FAN_SPEED_MIN); 
    }
  } 
  else { 
    // Night mode
    turnOffLight(); 
    if (temperature < (TEMPERATURE_NIGHT - TEMPERATURE_DELTA)) {
      // Too cold, Nothig to do, no heater
      turnOffFan(); 
    } 
    else if (temperature > (TEMPERATURE_NIGHT + 2*TEMPERATURE_DELTA)){
      // Too hot
      turnOnFan(FAN_SPEED_MAX);    
    } 
    else if (temperature > (TEMPERATURE_NIGHT + TEMPERATURE_DELTA)){
      // Too hot
      turnOnFan(FAN_SPEED_MIN);    
    } 
    else {
      // Normal, all devices are turned off
      turnOffFan(); 
    }
  }
}

void switchToDayMode(){
  if (g_isDayInGrowbox == true){
    return;
  }
  g_isDayInGrowbox = true;
  GB_Logger::logEvent(EVENT_MODE_DAY);

  checkGrowboxState();
}

void switchToNightMode(){
  if (g_isDayInGrowbox == false){
    return;
  }
  g_isDayInGrowbox = false;
  GB_Logger::logEvent(EVENT_MODE_NIGHT);

  checkGrowboxState();
}

/////////////////////////////////////////////////////////////////////
//                              SENSORS                            //
/////////////////////////////////////////////////////////////////////

void updateThermometerStatistics(){ // should return void
  GB_Thermometer::updateStatistics(); 
}

/////////////////////////////////////////////////////////////////////
//                              DEVICES                            //
/////////////////////////////////////////////////////////////////////

void turnOnLight(){
  if (digitalRead(LIGHT_PIN) == RELAY_ON){
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_ON);
  GB_Logger::logEvent(EVENT_LIGHT_ON);
}

void turnOffLight(){
  if (digitalRead(LIGHT_PIN) == RELAY_OFF){
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_OFF);  
  GB_Logger::logEvent(EVENT_LIGHT_OFF);
}

void turnOnFan(int speed){
  if (digitalRead(FAN_PIN) == RELAY_ON && digitalRead(FAN_SPEED_PIN) == speed){
    return;
  }
  digitalWrite(FAN_SPEED_PIN, speed);
  digitalWrite(FAN_PIN, RELAY_ON);

  if (speed == FAN_SPEED_MIN){
    GB_Logger::logEvent(EVENT_FAN_ON_MIN);
  } 
  else {
    GB_Logger::logEvent(EVENT_FAN_ON_MAX);
  }
}

void turnOffFan(){
  if (digitalRead(FAN_PIN) == RELAY_OFF){
    return;
  }
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);
  GB_Logger::logEvent(EVENT_FAN_OFF);
}


static void executeCommand(String &input){

  // send data only when you receive data:

  // read the incoming byte:
  char firstChar = 0, secondChar = 0; 
  firstChar = input[0];
  if (input.length() > 1){
    secondChar = input[1];
  }

  Serial.print(F("GB>"));
  Serial.print(firstChar);
  if (secondChar != 0){
    Serial.print(secondChar);
  }
  Serial.println();

  boolean printEvents = true; // can't put in switch, Arduino bug
  boolean printErrors = true;
  boolean printTemperature = true;

  switch(firstChar){
  case 's':
    printStatus(); 
    break;  

  case 'l':
    switch(secondChar){
    case 'c': 
      Serial.println(F("Stored log records cleaned"));
      GB_StorageHelper::resetStoredLog();
      break;
    case 'e':
      Serial.println(F("Logger store enabled"));
      GB_StorageHelper::setStoreLogRecordsEnabled(true);
      break;
    case 'd':
      Serial.println(F("Logger store disabled"));
      GB_StorageHelper::setStoreLogRecordsEnabled(false);
      break;
    case 't': 
      printErrors = printEvents = false;
      break;
    case 'r': 
      printEvents = printTemperature = false;
      break;
    case 'v': 
      printTemperature = printErrors = false;
      break;
    } 
    if ((secondChar != 'c') && (secondChar != 'e') && (secondChar != 'd')){
      GB_Logger::printFullLog(printEvents,  printErrors,  printTemperature );
    }
    break; 

  case 'b': 
    switch(secondChar){
    case 'c': 
      Serial.println(F("Cleaning boot record"));

      GB_StorageHelper::resetFirmware();
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
      Serial.println(F("Rebooting..."));
      GB_Controller::rebootController();
      break;
    } 

    Serial.println(F("Currnet boot record"));
    Serial.print(F("-Memory : ")); 
    {// TODO compilator madness
      BootRecord bootRecord = GB_StorageHelper::getBootRecord();
      GB_PrintDirty::printRAM(&bootRecord, sizeof(BootRecord));
      Serial.println();
    }
    Serial.print(F("-Storage: ")); 
    printStorage(0, sizeof(BootRecord));

    break;  

  case 'm':    
    switch(secondChar){
    case '0': 
      GB_Storage::fillStorage(0x00); 
      break; 
    case 'a': 
      GB_Storage::fillStorage(0xAA); 
      break; 
    case 'f': 
      GB_Storage::fillStorage(0xFF); 
      break; 
    case 'i': 
      GB_Storage::fillStorageIncremental(); 
      break; 
    }
    printStorage();
    break; 
  case 'r':        
    Serial.println(F("Rebooting..."));
    GB_Controller::rebootController();
    break; 
  default: 
    GB_Logger::logEvent(EVENT_SERIAL_UNKNOWN_COMMAND);  
  }
  delay(1000);               // wait for a second
  GB_SerialHelper::printDirtyEnd();
}


static void printFreeMemory(){
  Serial.print(F("Free memory: "));     // print how much RAM is available.
  Serial.print(freeMemory(), DEC);  // print how much RAM is available.
  Serial.println(F(" bytes"));     // print how much RAM is available.
}

void printStorage(word address, byte sizeOf){
  byte buffer[sizeOf];
  GB_Storage::read(address, buffer, sizeOf);
  GB_PrintDirty::printRAM(buffer, sizeOf);
  Serial.println();
}

void printStorage(){
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
      GB_PrintDirty::print2digitsHEX(i/16);
    }
    Serial.print(" ");
    GB_PrintDirty::print2digitsHEX(value);
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



// private:

static void printBootStatus(){
  Serial.print(F("Controller: frist startup time: ")); 
  GB_PrintDirty::printTime(GB_StorageHelper::getFirstStartupTimeStamp());
  Serial.print(F(", last startup time: ")); 
  GB_PrintDirty::printTime(GB_StorageHelper::getLastStartupTimeStamp());
  Serial.println();
  Serial.print(F("Logger: "));
  if (GB_StorageHelper::isStoreLogRecordsEnabled()){
    Serial.print(F("enabled"));
  } 
  else {
    Serial.print(F("disabled"));
  }
  Serial.print(F(", records: "));
  Serial.print(GB_StorageHelper::getLogRecordsCount());
  Serial.print('/');
  Serial.print(GB_StorageHelper::LOG_CAPACITY);
  if (GB_StorageHelper::isLogOverflow()){
    Serial.print(F(", overflow"));
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
  GB_PrintDirty::printTime(now());
  Serial.print(F(", up time: [")); 
  Serial.print(UP_HOUR); 
  Serial.print(F(":00], down time: ["));
  Serial.print(DOWN_HOUR);
  Serial.print(F(":00], "));
  Serial.println();
}

static void printTemperatureStatus(){
  float workingTemperature, statisticsTemperature;
  int statisticsCount;
  GB_Thermometer::getStatistics(workingTemperature, statisticsTemperature, statisticsCount);

  Serial.print(F("Temperature work:")); 
  Serial.print(workingTemperature);
  Serial.print(F(", count:")); 
  Serial.print(statisticsTemperature);
  Serial.print(F("(el:")); 
  Serial.print(statisticsCount);

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
      GB_PrintDirty::print2digits(i);
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

#ifdef GB_EXTRA_STRINGS
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
    case USE_SERIAL_MONOTOR_PIN: 
      Serial.print(F("Use serial monitor on(1)/off(0)"));
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
#endif
    Serial.println();
  }
}









