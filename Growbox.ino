#include <MemoryFree.h>;

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
#include "Print.h"
#include "Storage.h"
#include "LogRecord.h"
#include "BootRecord.h"
#include "Error.h"
#include "Event.h"
#include "Logger.h"
#include "SerialMonitor.h"

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////
String WIFI_SID = "Hell";
String WIFI_PASS = "flat65router";


// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWirePin(ONE_WIRE_PIN);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature g_dallasTemperature(&oneWirePin);
DeviceAddress g_thermometerOneWireAddress;


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

// http://electronics.stackexchange.com/questions/66983/how-to-discover-memory-overflow-errors-in-the-arduino-c-code
void checkFreeMemory(){
  if(freeMemory() < 200){ 
    GB_Logger::logError(ERROR_MEMORY_LOW);   
  } 
  else {
    GB_Logger::stopLogError(ERROR_MEMORY_LOW); 
  }
}

void checkUseSerialWifi(){

  // Check is Wi-fi present
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  } 

  String input="";

  // Clean Serial buffer
  while (Serial.available()){
    Serial.read();
  }

  Serial.println(F("at+reset=0"));
  delay(WIFI_DELAY);  
  while (Serial.available()){
    input += (char) Serial.read();
  }
  g_UseSerialWifi = WIFI_WELLCOME.equals(input);
  if(g_UseSerialWifi){
    Serial.println(F("Wi-Fi: enabled"));
  } 
  else {
    Serial.println(F("Wi-Fi: disabled"));
  }
  printEnd();

  g_UseSerial = true; // Serial not closed;
  g_UseSerialMonitor = true; // Monitor working
}

void checkUseSerialMonitor(){

  boolean useSerialMonitor = (digitalRead(USE_SERIAL_MONOTOR_PIN) == SERIAL_ON);
  if (useSerialMonitor == g_UseSerialMonitor){
    return;
  }
  g_UseSerialMonitor = useSerialMonitor;

  if (g_UseSerial){
    if (g_UseSerialMonitor){
      Serial.println(F("Serial monitor: enabled"));
    } 
    else {
      Serial.println(F("Serial monitor: disabled"));
    }
    printEnd();
  }

  boolean useSerial = g_UseSerialWifi || g_UseSerialMonitor;
  if (useSerial == g_UseSerial){
    return;
  }
  g_UseSerial = useSerial;

  if (g_UseSerial){
    Serial.begin(115200);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for Leonardo only
    }
    Serial.println(F("Serial: enabled"));
    if (g_UseSerialMonitor){
      Serial.println(F("Serial monitor: enabled"));
    }
    printEnd();
  } 
  else {
    Serial.println(F("Serial: disabled"));
    printEnd();
    Serial.end();
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

  // We need to check Wi-Fi before use Serial
  checkUseSerialWifi();
  checkUseSerialMonitor();

  // We should init Errors & Events before checkUseSerialWifi(), cause we may use them after
  if(g_UseSerialMonitor){ 
    //Serial.println();
    //Serial.println(F("Checking boot status"));  
    printFreeMemory();
    Serial.println(F("Checking software configuration"));
    printEnd();
  }

  initErrors();
  if (!Error::isInitialized()){
    if(g_UseSerialMonitor){ 
      Serial.print(F("Fatal error: not all Errors initialized"));
      printEnd();
    }
    while(true) delay(5000);  
  }
  initEvents();
  if (!Event::isInitialized()){
    if(g_UseSerialMonitor){ 
      Serial.print(F("Fatal error: not all Events initialized"));
      printEnd();
    }
    while(true) delay(5000);  
  }

  if (BOOT_RECORD_SIZE != sizeof(BootRecord)){
    digitalWrite(ERROR_PIN, HIGH);
    if(g_UseSerialMonitor){ 
      Serial.print(F("Fatal error: wrong BootRecord size. Exepted:"));
      Serial.print(BOOT_RECORD_SIZE);
      Serial.print(F(", current: "));
      Serial.print(sizeof(BootRecord));
      printEnd();
    }
    while(true) delay(5000);
  }

  if (LOG_RECORD_SIZE != sizeof(LogRecord)){
    digitalWrite(ERROR_PIN, HIGH);
    if(g_UseSerialMonitor){ 
      Serial.print(F("Fatal error: wrong LogRecord size. Exepted:"));
      Serial.print(BOOT_RECORD_SIZE);
      Serial.print(F(", current: "));
      Serial.print(sizeof(BootRecord));
      printEnd();
    }
    while(true) delay(5000);
  }

  checkFreeMemory();

  if(g_UseSerialMonitor){ 
    Serial.println(F("Checking clock"));
    printEnd();
  }

  // Configure clock
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  while(timeStatus() == timeNotSet || 2014<year() || year()>2020) { // ... and check it
    GB_Logger::logError(ERROR_TIMER_NOT_SET);
    setSyncProvider(RTC.get); // try to refresh clock
  }
  GB_Logger::stopLogError(ERROR_TIMER_NOT_SET); 

  checkFreeMemory();

  if(g_UseSerialMonitor){ 
    Serial.println(F("Checking termometer"));
    printEnd();
  }

  // Configure termometer
  g_dallasTemperature.begin();
  while(g_dallasTemperature.getDeviceCount() == 0){
    GB_Logger::logError(ERROR_TERMOMETER_DISCONNECTED);
    g_dallasTemperature.begin();
  }  
  GB_Logger::stopLogError(ERROR_TERMOMETER_DISCONNECTED);

  g_dallasTemperature.getAddress(g_thermometerOneWireAddress, 0); // search for devices on the bus and assign based on an index.
  while(!checkTemperature()) { // Load temperature on startup
    delay(1000);
  }

  checkFreeMemory();

  if(g_UseSerialMonitor){ 
    Serial.println(F("Checking storage"));
    printEnd();
  }

  // Check EEPROM, if Arduino doesn't reboot - all OK
  boolean isRestart = BOOT_RECORD.initOnStartup();

  if(g_UseSerialMonitor){ 
    Serial.println(F("Growbox successfully started"));
    printEnd();
  }

  // Now we can use logger
  if (isRestart){
    GB_Logger::logEvent(EVENT_RESTART);
  } 
  else {
    GB_Logger::logEvent(EVENT_FIRST_START_UP);
  }

  // Log current temeperature
  getTemperature();

  // Init/Restore growbox state
  if (isDayInGrowbox()){
    switchToDayMode();
  } 
  else {
    switchToNightMode();
  }

  // Create main life circle timer
  Alarm.timerRepeat(CHECK_TEMPERATURE_DELAY, checkTemperatureState);  // repeat every N seconds
  Alarm.timerRepeat(CHECK_GROWBOX_DELAY, checkGrowboxState);  // repeat every N seconds

  // Create suolemental rare switching
  Alarm.alarmRepeat(UP_HOUR, 00, 00, switchToDayMode);      // repeat once every day
  Alarm.alarmRepeat(DOWN_HOUR, 00, 00, switchToNightMode);  // repeat once every day

}

// the loop routine runs over and over again forever:
void loop() {

  digitalWrite(BREEZE_PIN, !digitalRead(BREEZE_PIN));

  checkFreeMemory();

  checkUseSerialMonitor();

  checkSerialCommands();

  Alarm.delay(MAIN_LOOP_DELAY * 1000); // wait one second between clock display
}


void serialEvent(){
  if(!BOOT_RECORD.isCorrect()){
    return; //Do not handle events during startup
  }

  String input = ""; 
  while (Serial.available()){
    input += (char) Serial.read();
  }
  Serial.print(F("Serial.read: "));
  Serial.println(input);
  printEnd();


  if (g_UseSerialWifi) {

  } 
  else if (g_UseSerialMonitor) {
    //checkSerialCommands();
  }


}

/////////////////////////////////////////////////////////////////////
//                  TIMER/CLOCK EVENT HANDLERS                     //
/////////////////////////////////////////////////////////////////////

void checkGrowboxState() {

  float temperature = getTemperature();

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

void checkTemperatureState(){ // should return void
  checkTemperature(); 
}

boolean checkTemperature(){

  if(!g_dallasTemperature.requestTemperaturesByAddress(g_thermometerOneWireAddress)){
    GB_Logger::logError(ERROR_TERMOMETER_DISCONNECTED);
    return false;
  };

  float freshTemperature = g_dallasTemperature.getTempC(g_thermometerOneWireAddress);

  if ((int)freshTemperature == 0){
    GB_Logger::logError(ERROR_TERMOMETER_ZERO_VALUE);  
    return false;
  }

  g_temperatureSumm += freshTemperature;
  g_temperatureSummCount++;

  boolean forceLog = 
    GB_Logger::stopLogError(ERROR_TERMOMETER_ZERO_VALUE) |
    GB_Logger::stopLogError(ERROR_TERMOMETER_DISCONNECTED); 
  if (forceLog) {
    getTemperature(true);
  }
  else if (g_temperatureSummCount > 100){
    getTemperature(); // prevents overflow 
  }

  return true;
}

float getTemperature(){
  getTemperature(false);
}

float getTemperature(boolean forceLog){

  if (g_temperatureSummCount == 0){
    return g_temperature; 
  }

  float freshTemperature = g_temperatureSumm/g_temperatureSummCount;

  if (((int)freshTemperature != (int)g_temperature) || forceLog) {          
    GB_Logger::logTemperature((byte)freshTemperature);
  }

  g_temperature = freshTemperature;

  g_temperatureSumm = 0.0;
  g_temperatureSummCount = 0;

  return g_temperature;
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


