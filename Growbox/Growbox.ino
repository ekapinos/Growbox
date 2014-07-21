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
#include "StorageHelper.h"
#include "Thermometer.h"
#include "RAK410_XBeeWifi.h"
#include "WebServer.h"

/////////////////////////////////////////////////////////////////////
//                              STATUS                             //
/////////////////////////////////////////////////////////////////////

boolean isDayInGrowbox(){
  if(timeStatus() == timeNeedsSync){
    GB_Logger.logError(ERROR_TIMER_NEEDS_SYNC);
  } 
  else {
    GB_Logger.stopLogError(ERROR_TIMER_NEEDS_SYNC);
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


void printStatusOnBoot(const __FlashStringHelper* str){ //TODO 
  if (g_useSerialMonitor){
    Serial.print(F("Checking "));
    Serial.print(str);
    Serial.println(F("..."));
  }
}

void stopOnFatalError(const __FlashStringHelper* str){ //TODO 
  digitalWrite(ERROR_PIN, HIGH);
  if (g_useSerialMonitor){
    Serial.print(F("Fatal error: "));
    Serial.println(str);
  }
  while(true) delay(5000); // Stop boot process
}

void printFreeMemory(){  
  Serial.print(FS(S_Free_memory));
  Serial.print(freeMemory()); 
  Serial.println(FS(S_bytes));
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
  
  // Hardware buttons
  pinMode(HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN, INPUT_PULLUP);
  pinMode(HARDWARE_BUTTON_RESET_FIRMWARE_PIN, INPUT_PULLUP);

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

  g_isGrowboxStarted = false;

  // We need to check Wi-Fi before use print to SerialMonitor
  int controllerFreeMemoryBeforeBoot = freeMemory();

  GB_Controller.updateHardwareInputStatus();
  RAK410_XBeeWifi.init();

  // We should init Errors & Events before checkSerialWifi->(), cause we may use them after
  if(g_useSerialMonitor){
    printFreeMemory();
    printStatusOnBoot(F("software configuration"));
  }

  initLoggerModel();
  if (!Error::isInitialized()){
    stopOnFatalError(F("not all Errors initialized"));
  }
  if (!Event::isInitialized()){
    stopOnFatalError(F("not all Events initialized"));
  }

  if (BOOT_RECORD_SIZE != sizeof(BootRecord)){
    Serial.print(BOOT_RECORD_SIZE);  
    Serial.print('-'); 
    Serial.print(sizeof(BootRecord));
    stopOnFatalError(F("wrong BootRecord size"));
  }

  if (LOG_RECORD_SIZE != sizeof(LogRecord)){
    stopOnFatalError(F("wrong LogRecord size"));
  }

  GB_Controller.checkFreeMemory();

  if(g_useSerialMonitor){ 
    printStatusOnBoot(F("clock"));
  }

  // Configure clock
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  while(timeStatus() == timeNotSet || 2014<year() || year()>2020) { // ... and check it
    GB_Logger.logError(ERROR_TIMER_NOT_SET);
    setSyncProvider(RTC.get); // try to refresh clock
  }
  GB_Logger.stopLogError(ERROR_TIMER_NOT_SET); 

  GB_Controller.checkFreeMemory();

  if(g_useSerialMonitor){ 
    printStatusOnBoot(F("termometer"));
  }

  // Configure termometer
  GB_Thermometer.init();
  while(!GB_Thermometer.updateStatistics()) { // Load temperature on startup
    delay(1000);
  }

  GB_Controller.checkFreeMemory();

  if(g_useSerialMonitor){ 
    printStatusOnBoot(F("storage"));
  }
  
  //GB_StorageHelper.resetFirmware();

  // Check EEPROM, if Arduino doesn't reboot - all OK
  boolean itWasRestart = GB_StorageHelper.init();

  g_isGrowboxStarted = true;

//
//  Serial.println(GB_StorageHelper.LOG_CAPACITY);
//  Serial.println(GB_StorageHelper.LOG_CAPACITY_ARDUINO);
//  Serial.println(GB_StorageHelper.LOG_CAPACITY_AT24C32);
//  
//  byte test [] ={200, 200, 200, 200, 200};
//  EEPROM_AT24C32.writeBlock<byte>(0, test, 5);
//
//  for (word i=0; i< GB_StorageHelper.LOG_CAPACITY; i++){
//    LogRecord logRecord(i);//(B11000000|(i%B00111111));
//    GB_StorageHelper.storeLogRecord(logRecord);
//    Serial.print(F("SERIAL HELPER> Write ")); 
//    Serial.println(i); 
//  }
//
//  for (word i=0; i< GB_StorageHelper.getLogRecordsCount(); i++){
//    LogRecord logRecord;
//    GB_StorageHelper.getLogRecordByIndex(i, logRecord);
//    Serial.print(i); 
//    Serial.print(" - ");     
//    Serial.print(StringUtils::timeToString(logRecord.timeStamp)); 
//    Serial.print(" - "); 
//    Serial.println(logRecord.data); 
//  }
//
//  while(true) delay(5000);
//  return;


  // Now we can use logger
  if (itWasRestart){
    GB_Logger.logEvent(EVENT_RESTART);
  } 
  else {
    GB_Logger.logEvent(EVENT_FIRST_START_UP);
  }

  // Log current temeperature
  GB_Thermometer.getTemperature(); // forceLog?

  // Init/Restore growbox state
  if (isDayInGrowbox()){
    switchToDayMode();
  } 
  else {
    switchToNightMode();
  }

  // Create main life circle timer
  Alarm.timerRepeat(UPDATE_THEMPERATURE_STATISTICS_DELAY, updateThermometerStatistics);  // repeat every N seconds
  Alarm.timerRepeat(UPDATE_SERIAL_MONITOR_STATUS_DELAY, updateControllerHardwareInputStatus);
  Alarm.timerRepeat(UPDATE_WIFI_STATUS_DELAY, updateWiFiStatus); 
  Alarm.timerRepeat(UPDATE_BREEZE_DELAY, updateBreezeStatus); 

  Alarm.timerRepeat(UPDATE_GROWBOX_STATE_DELAY, updateGrowboxState);  // repeat every N seconds

  // Create suolemental rare switching
  Alarm.alarmRepeat(UP_HOUR, 00, 00, switchToDayMode);      // repeat once every day
  Alarm.alarmRepeat(DOWN_HOUR, 00, 00, switchToNightMode);  // repeat once every day

  if(g_useSerialMonitor){ 
    if (controllerFreeMemoryBeforeBoot != freeMemory()){
      printFreeMemory();
    }
    Serial.println(F("Growbox successfully started"));
  }

  if (RAK410_XBeeWifi.isPresent()){ 
    RAK410_XBeeWifi.updateWiFiStatus(); // start Web server
  }

}

// the loop routine runs over and over again forever:
void loop() {
  //WARNING! We need quick response for Serial events, thay handled afer each loop. So, we decreese delay to zero 
  Alarm.delay(0); 
}

// RAK 410 is connected to Serial-1
void serialEvent1(){

  if(!g_isGrowboxStarted){
    // We will not handle external events during startup
    return;
  }

  GB_WebServer.handleSerialEvent();
}


/////////////////////////////////////////////////////////////////////
//                  TIMER/CLOCK EVENT HANDLERS                     //
/////////////////////////////////////////////////////////////////////

void updateGrowboxState() {

  GB_Controller.checkFreeMemory();

  float temperature = GB_Thermometer.getTemperature();

  if (temperature >= TEMPERATURE_CRITICAL){
    turnOffLight();
    turnOnFan(FAN_SPEED_MAX);
    GB_Logger.logError(ERROR_TERMOMETER_CRITICAL_VALUE);
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
  GB_Logger.logEvent(EVENT_MODE_DAY);

  updateGrowboxState();
}

void switchToNightMode(){
  if (g_isDayInGrowbox == false){
    return;
  }
  g_isDayInGrowbox = false;
  GB_Logger.logEvent(EVENT_MODE_NIGHT);

  updateGrowboxState();
}

/////////////////////////////////////////////////////////////////////
//                              SCHEDULE                           //
/////////////////////////////////////////////////////////////////////

void updateThermometerStatistics(){ // should return void
  GB_Thermometer.updateStatistics(); 
}

void updateWiFiStatus(){ // should return void
  RAK410_XBeeWifi.updateWiFiStatus(); 
}

void updateControllerHardwareInputStatus(){ // should return void
  GB_Controller.updateHardwareInputStatus(); 
}

void updateBreezeStatus() {
  digitalWrite(BREEZE_PIN, !digitalRead(BREEZE_PIN));
}

/////////////////////////////////////////////////////////////////////
//                              DEVICES                            //
/////////////////////////////////////////////////////////////////////


void turnOnLight(){
  if (digitalRead(LIGHT_PIN) == RELAY_ON){
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_ON);
  GB_Logger.logEvent(EVENT_LIGHT_ON);
}

void turnOffLight(){
  if (digitalRead(LIGHT_PIN) == RELAY_OFF){
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_OFF);  
  GB_Logger.logEvent(EVENT_LIGHT_OFF);
}

void turnOnFan(int speed){
  if (digitalRead(FAN_PIN) == RELAY_ON && digitalRead(FAN_SPEED_PIN) == speed){
    return;
  }
  digitalWrite(FAN_SPEED_PIN, speed);
  digitalWrite(FAN_PIN, RELAY_ON);

  if (speed == FAN_SPEED_MIN){
    GB_Logger.logEvent(EVENT_FAN_ON_MIN);
  } 
  else {
    GB_Logger.logEvent(EVENT_FAN_ON_MAX);
  }
}

void turnOffFan(){
  if (digitalRead(FAN_PIN) == RELAY_OFF){
    return;
  }
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);
  GB_Logger.logEvent(EVENT_FAN_OFF);
}








