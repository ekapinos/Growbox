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

  byte upHour, upMinute, downHour, downMinute;
  GB_StorageHelper.getTurnToDayAndNightTime(upHour, upMinute, downHour, downMinute);

  word upTime = upHour * 60 + upMinute;
  word downTime = downHour * 60 + downMinute;
  word currentTime = hour() * 60 + minute();

  boolean isDayInGrowbox = false;
  if (upTime < downTime){
    if (upTime < currentTime && currentTime < downTime ){
      isDayInGrowbox = true;
    }
  } 
  else { // upTime > downTime
    if (upTime < currentTime || currentTime < downTime ){
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

  // Watering
  pinMode(WATERING_SENSOR_ANALOG_PIN, INPUT_PULLUP);
  pinMode(WATERING1_SENSOR_ANALOG_PIN, INPUT_PULLUP);
  pinMode(WATERING2_SENSOR_ANALOG_PIN, INPUT_PULLUP);
  pinMode(WATERING3_SENSOR_ANALOG_PIN, INPUT_PULLUP);

  pinMode(WATERING_SENSOR_POWER_PIN, OUTPUT);
  pinMode(WATERING1_SENSOR_POWER_PIN, OUTPUT);
  pinMode(WATERING2_SENSOR_POWER_PIN, OUTPUT);
  pinMode(WATERING3_SENSOR_POWER_PIN, OUTPUT);

  // Reley pins
  pinMode(LIGHT_PIN, OUTPUT);   
  pinMode(FAN_PIN, OUTPUT);  
  pinMode(FAN_SPEED_PIN, OUTPUT); 
  
  pinMode(WATERING_PUMP_PIN, OUTPUT);
  pinMode(WATERING1_PUMP_PIN, OUTPUT);
  pinMode(WATERING2_PUMP_PIN, OUTPUT);
  pinMode(WATERING3_PUMP_PIN, OUTPUT);
  
  // Configure relay
  digitalWrite(LIGHT_PIN, RELAY_OFF);
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);
  
  digitalWrite(WATERING_PUMP_PIN, RELAY_OFF);
  digitalWrite(WATERING1_PUMP_PIN, RELAY_OFF);
  digitalWrite(WATERING2_PUMP_PIN, RELAY_OFF);
  digitalWrite(WATERING3_PUMP_PIN, RELAY_OFF);

  // Configure inputs
  //attachInterrupt(0, interrapton0handler, CHANGE); // PIN 2

  g_isGrowboxStarted = false;

  GB_Controller.updateHardwareInputStatus();
  RAK410_XBeeWifi.init();

  // We should init Errors & Events before checkSerialWifi->(), cause we may use them after
  if(g_useSerialMonitor){
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
    Serial.print(F("Expected: "));
    Serial.print(BOOT_RECORD_SIZE);  
    Serial.print(F("configured: ")); 
    Serial.println(sizeof(BootRecord));
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

  // Check EEPROM, if Arduino doesn't reboot - all OK
  boolean itWasRestart = GB_StorageHelper.init();

  g_isGrowboxStarted = true;

  // Now we can use logger
  if (itWasRestart){
    GB_Logger.logEvent(EVENT_RESTART);
  } 
  else {
    GB_Logger.logEvent(EVENT_FIRST_START_UP);
  }

  GB_Controller.checkFreeMemory();

  updateGrowboxState();
  // Log current temeperature
  //GB_Thermometer.getTemperature(); // forceLog?

  GB_Controller.checkFreeMemory();

  // Create main life circle timer
  Alarm.timerRepeat(UPDATE_THEMPERATURE_STATISTICS_DELAY, updateThermometerStatistics);  // repeat every N seconds
  Alarm.timerRepeat(UPDATE_SERIAL_MONITOR_STATUS_DELAY, updateControllerHardwareInputStatus);
  Alarm.timerRepeat(UPDATE_WIFI_STATUS_DELAY, updateWiFiStatus); 
  Alarm.timerRepeat(UPDATE_BREEZE_DELAY, updateBreezeStatus); 

  Alarm.timerRepeat(UPDATE_GROWBOX_STATE_DELAY, updateGrowboxState);  // repeat every N seconds

  // TODO update if configuration changed
  // Create suolemental rare switching
  //Alarm.alarmRepeat(upHour, upMinute, 00, switchToDayMode);      // repeat once every day
  //Alarm.alarmRepeat(downHour, downMinute, 00, switchToNightMode);  // repeat once every day

  GB_Controller.checkFreeMemory();

  if(g_useSerialMonitor){ 
    Serial.println(F("Growbox successfully started"));
  }

  if (RAK410_XBeeWifi.isPresent()){ 
    RAK410_XBeeWifi.updateWiFiStatus(); // start Web server
  }

  GB_Controller.checkFreeMemory();

}

// the loop routine runs over and over again forever:
void loop() {
  //WARNING! We need quick response for Serial events, thay handled afer each loop. So, we decreese delay to zero 
  Alarm.delay(0); 
}

void serialEvent(){
  if(!g_isGrowboxStarted){
    // We will not handle external events during startup
    return;
  }
  String input;
  while (Serial.available()){
    input += (char) Serial.read();
  }
  int indexOfQestionChar = input.indexOf('?');
  if (indexOfQestionChar < 0){
    if(g_useSerialMonitor){ 
      Serial.println(F("Wrong serial command. '?' char not found. Syntax: url?param1=value1[&param2=value]"));
    }
    return;
  }
  
  if (StringUtils::flashStringEndsWith(input, FS(S_CRLF))){
    input = input.substring(0, input.length()-2);   
  }
  
  String url(input);
  url.substring(0, indexOfQestionChar);
  input.substring(indexOfQestionChar+1);
  
  if(g_useSerialMonitor){ 
    Serial.print(F("Recive from [SM] POST ["));
    Serial.print(url);
    Serial.print(F("] postParams ["));
    Serial.print(input);   
    Serial.println(']');
  }
  
}

// Wi-Fi is connected to Serial1
void serialEvent1(){

  if(!g_isGrowboxStarted){
    // We will not handle external events during startup
    return;
  }
  //  String input;
  //  Serial_readString(input); // at first we should read, after manipulate  
  //  Serial.println(input);
  boolean forceUpdate = GB_WebServer.handleSerialEvent();
  if (forceUpdate){
    updateGrowboxState();
  }
}


/////////////////////////////////////////////////////////////////////
//                  TIMER/CLOCK EVENT HANDLERS                     //
/////////////////////////////////////////////////////////////////////

void updateGrowboxState() {

  // Init/Restore growbox state
  if (isDayInGrowbox()){
    if (g_isDayInGrowbox != true){
      g_isDayInGrowbox = true;
      GB_Logger.logEvent(EVENT_MODE_DAY);
    }
  } 
  else {
    if (g_isDayInGrowbox != false){
      g_isDayInGrowbox = false;
      GB_Logger.logEvent(EVENT_MODE_NIGHT);
    }
  }

  byte normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue;
  GB_StorageHelper.getTemperatureParameters(normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue);

  float temperature = GB_Thermometer.getTemperature();

  if (temperature >= criticalTemperatue){
    turnOffLight();
    turnOnFan(FAN_SPEED_MAX);
    GB_Logger.logError(ERROR_TERMOMETER_CRITICAL_VALUE);
  } 
  else if (g_isDayInGrowbox) {
    // Day mode
    turnOnLight();
    if (temperature < normalTemperatueDayMin) {
      // Too cold, no heater
      turnOnFan(FAN_SPEED_MIN); // no wind, no grow
    } 
    else if (temperature > normalTemperatueDayMax){
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
    if (temperature < normalTemperatueNightMin) {
      // Too cold, Nothig to do, no heater
      turnOffFan(); 
    } 
    else if (temperature > normalTemperatueNightMax){
      // Too hot
      turnOnFan(FAN_SPEED_MIN);    
    } 
    else {
      // Normal, all devices are turned off
      turnOffFan(); 
    }
  }
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


