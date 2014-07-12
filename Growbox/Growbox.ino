
//#include <EEPROM.h>
#include <stdint.h>

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
    //RAK410_XBeeWifi.printDirtyEnd();
  }
}

void printFatalErrorOnBoot(const __FlashStringHelper* str){ //TODO 
  if (g_useSerialMonitor){
    Serial.print(F("Fatal error: "));
    Serial.println(str);
    //RAK410_XBeeWifi.printDirtyEnd();
  }
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

  g_isGrowboxStarted = false;

  // We need to check Wi-Fi before use print to SerialMonitor
  int controllerFreeMemoryBeforeBoot = freeMemory();

  GB_Controller.updateSerialMonitorStatus();
  RAK410_XBeeWifi.checkSerial();

  // We should init Errors & Events before checkSerialWifi->(), cause we may use them after
  if(g_useSerialMonitor){
    PrintUtils::printFreeMemory();
    printStatusOnBoot(F("software configuration"));
    //RAK410_XBeeWifi.printDirtyEnd();
  }

  initLoggerModel();
  if (!Error::isInitialized()){
    if(g_useSerialMonitor){ 
      printFatalErrorOnBoot(F("not all Errors initialized"));
      //RAK410_XBeeWifi.printDirtyEnd();
    }
    while(true) delay(5000);  
  }
  if (!Event::isInitialized()){
    if(g_useSerialMonitor){ 
      printFatalErrorOnBoot(F("not all Events initialized"));
      //RAK410_XBeeWifi.printDirtyEnd();
    }
    while(true) delay(5000);  
  }

  if (BOOT_RECORD_SIZE != sizeof(BootRecord)){
    digitalWrite(ERROR_PIN, HIGH);
    if(g_useSerialMonitor){ 
      printFatalErrorOnBoot(F("wrong BootRecord size"));
      //RAK410_XBeeWifi.printDirtyEnd();
    }
    while(true) delay(5000);
  }

  if (LOG_RECORD_SIZE != sizeof(LogRecord)){
    digitalWrite(ERROR_PIN, HIGH);
    if(g_useSerialMonitor){ 
      printFatalErrorOnBoot(F("wrong LogRecord size"));
      //RAK410_XBeeWifi.printDirtyEnd();
    }
    while(true) delay(5000);
  }

  GB_Controller.checkFreeMemory();

  if(g_useSerialMonitor){ 
    printStatusOnBoot(F("clock"));
    //RAK410_XBeeWifi.printDirtyEnd();
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
    //RAK410_XBeeWifi.printDirtyEnd();
  }

  // Configure termometer
  GB_Thermometer::start();
  while(!GB_Thermometer::updateStatistics()) { // Load temperature on startup
    delay(1000);
  }

  GB_Controller.checkFreeMemory();

  if(g_useSerialMonitor){ 
    printStatusOnBoot(F("storage"));
    //RAK410_XBeeWifi.printDirtyEnd();
  }

  // Check EEPROM, if Arduino doesn't reboot - all OK
  boolean itWasRestart = GB_StorageHelper::start();

  g_isGrowboxStarted = true;

  // Now we can use logger
  if (itWasRestart){
    GB_Logger.logEvent(EVENT_RESTART);
  } 
  else {
    GB_Logger.logEvent(EVENT_FIRST_START_UP);
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

  // Create main life circle timer
  Alarm.timerRepeat(UPDATE_THEMPERATURE_STATISTICS_DELAY, updateThermometerStatistics);  // repeat every N seconds
  Alarm.timerRepeat(UPDATE_SERIAL_MONITOR_STATUS_DELAY, updateSerialMonitorStatus);
  Alarm.timerRepeat(UPDATE_WIFI_STATUS_DELAY, updateWiFiStatus); 
  Alarm.timerRepeat(UPDATE_BREEZE_DELAY, updateBreezeStatus); 

  Alarm.timerRepeat(UPDATE_GROWBOX_STATE_DELAY, updateGrowboxState);  // repeat every N seconds

  // Create suolemental rare switching
  Alarm.alarmRepeat(UP_HOUR, 00, 00, switchToDayMode);      // repeat once every day
  Alarm.alarmRepeat(DOWN_HOUR, 00, 00, switchToNightMode);  // repeat once every day

  if(g_useSerialMonitor){ 
    if (controllerFreeMemoryBeforeBoot != freeMemory()){
      PrintUtils::printFreeMemory();
    }
    Serial.println(F("Growbox successfully started"));
  }

  if (RAK410_XBeeWifi.useSerialWifi){ 
    RAK410_XBeeWifi.setWifiConfiguration(StringUtils::flashStringLoad(F("Hell")), StringUtils::flashStringLoad(F("flat65router"))); 
    RAK410_XBeeWifi.startWifi();
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

  float temperature = GB_Thermometer::getTemperature();

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
  GB_Thermometer::updateStatistics(); 
}

void updateWiFiStatus(){ // should return void
  RAK410_XBeeWifi.updateWiFiStatus(); 
}

void updateSerialMonitorStatus(){ // should return void
  GB_Controller.updateSerialMonitorStatus(); 
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







