// Warning! We need to include all used libraries,
// otherwise Arduino IDE doesn't set correct build
// params for gcc compilator
#include <MemoryFree.h>

#include <Time.h>

#define USE_SPECIALIST_METHODS // We use free() method in TimeAlarms.h library
#include <TimeAlarms.h>

// RTC
#include <Wire.h>
#include <DS1307RTC.h>

// Thermometer
#include <OneWire.h>
#include <DallasTemperature.h>

// Growbox
#include "Global.h"
#include "Controller.h"
#include "StorageHelper.h"
#include "Thermometer.h"
#include "RAK410_XBeeWifi.h"
#include "WebServer.h"
#include "Watering.h"

/////////////////////////////////////////////////////////////////////
//                              STATUS                             //
/////////////////////////////////////////////////////////////////////

boolean isDayInGrowbox() {

  word upTime, downTime;
  GB_StorageHelper.getTurnToDayAndNightTime(upTime, downTime);

  word currentTime = hour() * 60 + minute();

  boolean isDayInGrowbox = false;
  if (upTime < downTime) {
    if (upTime < currentTime && currentTime < downTime) {
      isDayInGrowbox = true;
    }
  }
  else { // upTime > downTime
    if (upTime < currentTime || currentTime < downTime) {
      isDayInGrowbox = true;
    }
  }
  return isDayInGrowbox;
}

void printStatusOnBoot(const __FlashStringHelper* str) { //TODO
  if (g_useSerialMonitor) {
    Serial.print(F("Checking "));
    Serial.print(str);
    Serial.println(F("..."));
  }
}

void stopOnFatalError(const __FlashStringHelper* str) { //TODO
  digitalWrite(ERROR_PIN, HIGH);
  if (g_useSerialMonitor) {
    Serial.print(F("Fatal error: "));
    Serial.println(str);
  }
  while (true) delay(5000); // Stop boot process
}

/////////////////////////////////////////////////////////////////////
//                                MAIN                             //
/////////////////////////////////////////////////////////////////////

// the setup routine runs once when you press reset:
void setup() {

  // Initialize the info pins
  pinMode(LED_BUILTIN, OUTPUT); // Breeze
  pinMode(BREEZE_PIN, OUTPUT);  // Breeze
  pinMode(ERROR_PIN, OUTPUT);

  // Hardware buttons
  pinMode(HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN, INPUT_PULLUP);
  pinMode(HARDWARE_BUTTON_RESET_FIRMWARE_PIN, INPUT_PULLUP);

  // Relay pins
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(FAN_SPEED_PIN, OUTPUT);

  // Configure relay
  digitalWrite(LIGHT_PIN, RELAY_OFF);
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);

  // Watering
  for (int i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++) {
    pinMode(WATERING_WET_SENSOR_IN_PINS[i], INPUT_PULLUP);

    pinMode(WATERING_WET_SENSOR_POWER_PINS[i], OUTPUT);
    pinMode(WATERING_PUMP_PINS[i], OUTPUT);

    digitalWrite(WATERING_WET_SENSOR_POWER_PINS[i], HIGH);
    digitalWrite(WATERING_PUMP_PINS[i], RELAY_OFF);
  }
  unsigned long startupMillis = millis();

  // Configure inputs
  //attachInterrupt(0, interrapton0handler, CHANGE); // PIN 2

  GB_Controller.checkInputPinsStatus(true); // Check for Serial monitor and Firmware reset
  GB_Controller.checkFreeMemory();
  if (g_useSerialMonitor) {
    Serial.print(F("Build version: "));
    Serial.print(__DATE__);
    Serial.print(' ');
    Serial.print(__TIME__);
    Serial.println();
  }
  printStatusOnBoot(F("software configuration"));
  initLoggerModel();
  if (!Event::isInitialized()) {
    stopOnFatalError(F("not all Events initialized"));
  }
  if (!WateringEvent::isInitialized()) {
    stopOnFatalError(F("not all Watering Events initialized"));
  }
  if (!Error::isInitialized()) {
    stopOnFatalError(F("not all Errors initialized"));
  }
  if (BOOT_RECORD_SIZE != sizeof(BootRecord)) {
    if (g_useSerialMonitor) {
      Serial.print(F("Expected "));
      Serial.print(BOOT_RECORD_SIZE);
      Serial.print(F(" bytes, used "));
      Serial.println(sizeof(BootRecord));
      Serial.print(F(" bytes"));
    }
    stopOnFatalError(F("wrong BootRecord size"));
  }
  if (MAX_WATERING_SYSTEMS_COUNT != sizeof(WATERING_WET_SENSOR_IN_PINS)) {
    stopOnFatalError(F("wrong WATERING_WET_SENSOR_IN_PINS size"));
  }
  if (MAX_WATERING_SYSTEMS_COUNT != sizeof(WATERING_WET_SENSOR_POWER_PINS)) {
    stopOnFatalError(F("wrong WATERING_WET_SENSOR_POWER_PINS size"));
  }
  if (MAX_WATERING_SYSTEMS_COUNT != sizeof(WATERING_PUMP_PINS)) {
    stopOnFatalError(F("wrong WATERING_PUMP_PINS size"));
  }
  GB_Controller.checkFreeMemory();

  // Booted up

  printStatusOnBoot(F("clock"));
  time_t autoCalculatedTimeStamp = GB_StorageHelper.init_getLastStoredTime(); // returns zero, if first startup
  if (autoCalculatedTimeStamp != 0) {
    autoCalculatedTimeStamp += SECS_PER_MIN; // if not first start, we set +minute as defaul time
  }
  GB_Controller.initClock(autoCalculatedTimeStamp); // may be disconnected - it is OK, then we will use last log time
  GB_Controller.checkFreeMemory();

  time_t startupTimeStamp = now() - (millis() - startupMillis) / 1000;

  printStatusOnBoot(F("stored configuration"));
  GB_StorageHelper.init_loadConfiguration(startupTimeStamp); // Logger will enabled after that   // after set clock and load configuration we are ready for logging
  GB_Controller.checkFreeMemory();

  GB_Controller.initClock_afterLoadConfiguration(); // Save Auticalculated flag
  GB_Controller.checkFreeMemory();

  GB_Watering.init(startupTimeStamp); // call before updateGrowboxState();
  GB_Controller.checkFreeMemory();

  //  No need to call
  //  GB_Thermometer.updateStatistics();
  //  GB_Controller.checkFreeMemory();

  // Max 6 timer for Alarm instance
  Alarm.timerRepeat(UPDATE_GROWBOX_STATE_DELAY, updateGrowboxState);
  Alarm.timerRepeat(UPDATE_CONTROLLER_STATE_DELAY, updateControllerStatus);
  Alarm.timerRepeat(UPDATE_CONTROLLER_CORE_HARDWARE_STATE_DELAY, updateGrowboxCoreHardwareState);
  Alarm.timerRepeat(UPDATE_CONTROLLER_AUTO_ADJUST_CLOCK_TIME_DELAY, updateGrowboxAutoAdjustClockTime);
  Alarm.timerRepeat(UPDATE_TERMOMETER_STATISTICS_DELAY, updateThermometerStatistics);
  Alarm.timerRepeat(UPDATE_WEB_SERVER_STATUS_DELAY, updateWebServerStatus);
  GB_Controller.checkFreeMemory();

  updateGrowboxState();
  GB_Controller.checkFreeMemory();

  GB_WebServer.init(); // after load configuration
  GB_Controller.checkFreeMemory();

  GB_Watering.updateWateringSchedule(); // Run watering after init
  GB_Controller.checkFreeMemory();

  if (g_useSerialMonitor) {
    Serial.println(F("Growbox successfully started"));
  }

}

// the loop routine runs over and over again forever:
void loop() {
  //WARNING! We need quick response for Serial events, thay handled afer each loop. So, we decreese delay to zero
  Alarm.delay(0);

  // We use another instance of Alarm object to increase MAX alarms count (6 by default, look dtNBR_ALARMS in TimeAlarms.h
  GB_Watering.updateAlarms();
}

void serialEvent() {
  //  Serial.println(F("serialEvent fired"));
  //  if(!GB_StorageHelper.isConfigurationLoaded()){ //TODO maybe we should remove it
  //    // We will not handle external events during startup
  //    return;
  //  }

  boolean forceUpdateGrowboxState = GB_WebServer.handleSerialMonitorEvent();
  if (forceUpdateGrowboxState) {
    updateGrowboxState(false);
  }
}

// Wi-Fi is connected to Serial1
void serialEvent1() {
  //  Serial.println(F("serialEvent1 fired"));
  //  if(!GB_StorageHelper.isConfigurationLoaded()){
  //    // We will not handle external events during startup
  //    return;
  //  }

  //  String input;
  //  Serial_readString(input); // at first we should read, after manipulate
  //  Serial.println(input);
  boolean forceUpdateGrowboxState = GB_WebServer.handleSerialEvent();
  if (forceUpdateGrowboxState) {
    updateGrowboxState(false);
  }
}

/////////////////////////////////////////////////////////////////////
//                  TIMER/CLOCK EVENT HANDLERS                     //
/////////////////////////////////////////////////////////////////////

void updateGrowboxState() {
  updateGrowboxState(true);
}
void updateGrowboxState(boolean checkWetSensors) {

  if (checkWetSensors) {
    // Allows impruve stability on WEB call
    GB_Watering.preUpdateWetSatus();
  }

  // Init/Restore growbox state
  if (isDayInGrowbox()) {
    if (g_isDayInGrowbox != true) {
      g_isDayInGrowbox = true;
      GB_Logger.logEvent(EVENT_MODE_DAY);
    }
  }
  else {
    if (g_isDayInGrowbox != false) {
      g_isDayInGrowbox = false;
      GB_Logger.logEvent(EVENT_MODE_NIGHT);
    }
  }

  byte normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin,
      normalTemperatueNightMax, criticalTemperatue;
  GB_StorageHelper.getTemperatureParameters(normalTemperatueDayMin, normalTemperatueDayMax, normalTemperatueNightMin, normalTemperatueNightMax, criticalTemperatue);

  float temperature = GB_Thermometer.getTemperature();

  if (/*!isnan(temperature) &&*/temperature >= criticalTemperatue) {
    GB_Controller.turnOffLight();
    GB_Controller.turnOnFan(FAN_SPEED_MAX);
    GB_Logger.logError(ERROR_TERMOMETER_CRITICAL_VALUE);
  }
  else if (g_isDayInGrowbox) {
    // Day mode
    GB_Controller.turnOnLight();
    if (/*!isnan(temperature) && */temperature < normalTemperatueDayMin) {
      // Too cold, no heater
      GB_Controller.turnOnFan(FAN_SPEED_MIN); // no wind, no grow
    }
    else if (/*!isnan(temperature) &&*/temperature > normalTemperatueDayMax) {
      // Too hot
      GB_Controller.turnOnFan(FAN_SPEED_MAX);
    }
    else {
      // Normal, day wind
      GB_Controller.turnOnFan(FAN_SPEED_MIN);
    }
  }
  else {
    // Night mode
    GB_Controller.turnOffLight();
    if (/*!isnan(temperature) &&*/temperature < normalTemperatueNightMin) {
      // Too cold, Nothig to do, no heater
      GB_Controller.turnOffFan();
    }
    else if (/*!isnan(temperature) &&*/temperature > normalTemperatueNightMax) {
      // Too hot
      GB_Controller.turnOnFan(FAN_SPEED_MIN);
    }
    else {
      // Normal, all devices are turned off
      GB_Controller.turnOffFan();
    }
  }

  if (checkWetSensors) {
    GB_Watering.updateWetSatus(); // log new sensors values
  }
  GB_Watering.updateWateringSchedule(); // recalculate
}

/////////////////////////////////////////////////////////////////////
//                              SCHEDULE                           //
/////////////////////////////////////////////////////////////////////

void updateThermometerStatistics() { // should return void
  GB_Thermometer.updateStatistics();
}

void updateWebServerStatus() { // should return void
  GB_WebServer.update();
}

void updateControllerStatus() { // should return void
  GB_Controller.update(); // Check serial monitor without Firmware reset
}

void updateGrowboxCoreHardwareState() {
  GB_Controller.updateClockState();
  GB_StorageHelper.check_AT24C32_EEPROM();
}

void updateGrowboxAutoAdjustClockTime() {
  GB_Controller.updateAutoAdjustClockTime();
}

