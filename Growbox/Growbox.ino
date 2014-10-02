// Warning! We need to include all used libraries,
// otherwise Arduino IDE doesn't set correct build
// parameters for gcc compiler
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

void printStatusOnBoot(const __FlashStringHelper* str) {
  if (g_useSerialMonitor) {
    Serial.print(F("Checking "));
    Serial.print(str);
    Serial.println(F("..."));
  }
}

void stopOnFatalError(const __FlashStringHelper* str) {
  digitalWrite(ERROR_PIN, HIGH);
  if (g_useSerialMonitor) {
    Serial.print(F("Fatal error: "));
    Serial.println(str);
  }
  while (true) {
    // Stop boot process
  };
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

  // Configure relay pins (before set OUTPUT!)
  digitalWrite(LIGHT_PIN, RELAY_OFF);
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);
  digitalWrite(HEATER_PIN, RELAY_OFF);

  // Relay pins
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(FAN_SPEED_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);

  // Watering pins
  for (int i = 0; i < MAX_WATERING_SYSTEMS_COUNT; i++) {
    pinMode(WATERING_WET_SENSOR_IN_PINS[i], INPUT_PULLUP);

    // Configure relay pins (before set OUTPUT!)
    digitalWrite(WATERING_WET_SENSOR_POWER_PINS[i], HIGH);
    digitalWrite(WATERING_PUMP_PINS[i], RELAY_OFF);

    pinMode(WATERING_WET_SENSOR_POWER_PINS[i], OUTPUT);
    pinMode(WATERING_PUMP_PINS[i], OUTPUT);
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
  GB_Controller.numeratorDenominatorCombinationsCount(); // We should not trap into infinite loop
  GB_Controller.checkFreeMemory();

  // On this point system pass all fatal checks

  printStatusOnBoot(F("clock"));
  time_t lastStoredTimeStamp = GB_StorageHelper.init_getLastStoredTime(); // returns zero, if first startup
  GB_Controller.initClock(lastStoredTimeStamp); // may be disconnected - it is OK, then we will use last log time
  GB_Controller.checkFreeMemory();

  time_t startupTimeStamp = now() - (millis() - startupMillis) / 1000;

  printStatusOnBoot(F("stored configuration"));
  GB_StorageHelper.init_loadConfiguration(startupTimeStamp); // Logger will enabled after that. After set clock and load configuration we are ready for logging
  GB_Controller.checkFreeMemory();

  GB_Controller.initClock_afterLoadConfiguration(); // Save 'auto calculated' flag
  GB_Controller.checkFreeMemory();

  GB_Watering.init(startupTimeStamp); // call before updateGrowboxState();
  GB_Controller.checkFreeMemory();

  // WARNING! Max 6 timer for Alarm instance
  Alarm.timerRepeat(UPDATE_GROWBOX_STATE_DELAY_SEC, updateGrowboxState);
  Alarm.timerRepeat(UPDATE_CONTROLLER_STATE_DELAY_SEC, updateControllerStatus);
  Alarm.timerRepeat(UPDATE_CONTROLLER_CORE_HARDWARE_STATE_DELAY_SEC, updateGrowboxCoreHardwareState);
  Alarm.timerRepeat(UPDATE_TERMOMETER_STATISTICS_DELAY_SEC, updateThermometerStatistics);
  Alarm.timerRepeat(UPDATE_WEB_SERVER_STATUS_DELAY_SEC, updateWebServerStatus);
  Alarm.alarmRepeat(UPDATE_CONTROLLER_AUTO_ADJUST_CLOCK_TIME_SEC, updateGrowboxAutoAdjustClockTime); // once a day
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
  //WARNING! We need quick response for Serial events, they handled after each loop. So, we decrease delay to zero
  Alarm.delay(0);

  // We use another instance of Alarm object to increase MAX alarms count (6 by default, look dtNBR_ALARMS in TimeAlarms.h
  GB_Watering.updateAlarms();
}

// Arduino IDE is connected to Serial and works on standard 9600 speed
void serialEvent() {
  boolean forceUpdateGrowboxState = GB_WebServer.handleSerialMonitorEvent();
  if (forceUpdateGrowboxState) {
    updateGrowboxState(false);
  }
}

// Wi-Fi is connected to Serial1
void serialEvent1() {
  boolean forceUpdateGrowboxState = GB_WebServer.handleSerialWiFiEvent();
  if (forceUpdateGrowboxState) {
    updateGrowboxState(false);
  }
}

/////////////////////////////////////////////////////////////////////
//                  TIMER/CLOCK EVENT HANDLERS                     //
/////////////////////////////////////////////////////////////////////

void updateGrowboxState() {
  // On schedule we check wet status also
  updateGrowboxState(true);
}
void updateGrowboxState(boolean checkHardwareState) {

  if (checkHardwareState) {
    // Allows improve stability on WEB call
    GB_Watering.preUpdateWetSatus();
  }

  // Initialize/restore Growbox state
  boolean isDayInGrowbox = GB_Controller.isDayInGrowbox(true);

  byte normalTemperatueDayMin, normalTemperatueDayMax,
    normalTemperatueNightMin, normalTemperatueNightMax,
    criticalTemperatueMin, criticalTemperatueMax;
  GB_StorageHelper.getTemperatureParameters(
    normalTemperatueDayMin, normalTemperatueDayMax,
    normalTemperatueNightMin, normalTemperatueNightMax,
    criticalTemperatueMin, criticalTemperatueMax);

  byte fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
       fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature,fanSpeedNightHotTemperature;
  GB_StorageHelper.getFanParameters(
      fanSpeedDayColdTemperature, fanSpeedDayNormalTemperature, fanSpeedDayHotTemperature,
      fanSpeedNightColdTemperature, fanSpeedNightNormalTemperature, fanSpeedNightHotTemperature);

  // WARNING! May return NaN. Compare NaN with other numbers always 'false'
  float temperature;
  if (checkHardwareState && GB_Controller.isCurrentFanCycleFinished() ) {
    temperature = GB_Thermometer.getTemperatureAndClearStatistics();
  } else {
    temperature = GB_Thermometer.getLastTemperature();
  }
  if (checkHardwareState) {
    GB_Controller.setNextFanCycleStep();
  }

  if (temperature > criticalTemperatueMax) {
    // Critical Hot
    GB_Controller.turnOffLight();
    GB_Controller.turnOnFan(FAN_SPEED_HIGH);
    GB_Controller.turnOffHeater();
  }
  else if (temperature < criticalTemperatueMin) {
    // Critical Cold
    if (isDayInGrowbox) {
      GB_Controller.turnOnLight();
    } else {
      GB_Controller.turnOffLight();
    }
    GB_Controller.turnOffFan();
    GB_Controller.turnOnHeater();
  }
  else {
    // Not critical
    if (isDayInGrowbox) {
      // Day mode
      GB_Controller.turnOnLight();
      if (temperature < normalTemperatueDayMin) {
        // Cold (may heat by light)
        GB_Controller.turnOnOffFanBySpeedValue(fanSpeedDayColdTemperature);
        GB_Controller.turnOnHeater();
      }
      else if (temperature > normalTemperatueDayMax) {
        // Hot
        GB_Controller.turnOnOffFanBySpeedValue(fanSpeedDayHotTemperature);
        GB_Controller.turnOffHeater();
      }
      else {
        // Normal (or NAN)
        GB_Controller.turnOnOffFanBySpeedValue(fanSpeedDayNormalTemperature);
        GB_Controller.turnOffHeater();
      }
    }
    else {
      // Night mode
      GB_Controller.turnOffLight();
      if (temperature < normalTemperatueNightMin) {
        // Cold
        GB_Controller.turnOnOffFanBySpeedValue(fanSpeedNightColdTemperature);
        GB_Controller.turnOnHeater();
      }
      else if (temperature > normalTemperatueNightMax) {
        // Hot
        GB_Controller.turnOnOffFanBySpeedValue(fanSpeedNightHotTemperature);
        GB_Controller.turnOffHeater();
      }
      else {
        // Normal (or NAN)
        GB_Controller.turnOnOffFanBySpeedValue(fanSpeedNightNormalTemperature);
        GB_Controller.turnOffHeater();
      }
    }
  }

  if (checkHardwareState) {
    GB_Watering.updateWetSatus(); // log new sensors values
  }
  GB_Watering.updateWateringSchedule(); // recalculate next watering pump event
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

