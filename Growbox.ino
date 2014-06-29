
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


/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

byte g_isDayInGrowbox = -1;

// HTTP response supplemental  
byte g_isWifiRequest = false;  
byte g_wifiPortDescriptor = 0xFF;  

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

static void printSendBootStatus(const __FlashStringHelper* str){ //TODO 
  if (GB_SerialHelper::useSerialMonitor){
    Serial.print(F("Checking "));
    Serial.print(str);
    Serial.println(F("..."));
    //GB_SerialHelper::printDirtyEnd();
  }
}

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

  GB_SerialHelper::checkSerial(true, true);

  // We should init Errors & Events before checkSerialWifi->(), cause we may use them after
  if(GB_SerialHelper::useSerialMonitor){
    printSendFreeMemory();
    printSendBootStatus(F("software configuration"));
    //GB_SerialHelper::printDirtyEnd();
  }

  initLoggerModel();
  if (!Error::isInitialized()){
    if(GB_SerialHelper::useSerialMonitor){ 
      Serial.print(F("Fatal error: not all Errors initialized"));
      //GB_SerialHelper::printDirtyEnd();
    }
    while(true) delay(5000);  
  }
  if (!Event::isInitialized()){
    if(GB_SerialHelper::GB_SerialHelper::useSerialMonitor){ 
      Serial.print(F("Fatal error: not all Events initialized"));
      //GB_SerialHelper::printDirtyEnd();
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
      //GB_SerialHelper::printDirtyEnd();
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
      //GB_SerialHelper::printDirtyEnd();
    }
    while(true) delay(5000);
  }

  GB_Controller::checkFreeMemory();

  if(GB_SerialHelper::useSerialMonitor){ 
    printSendBootStatus(F("clock"));
    //GB_SerialHelper::printDirtyEnd();
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
    printSendBootStatus(F("termometer"));
    //GB_SerialHelper::printDirtyEnd();
  }

  // Configure termometer
  GB_Thermometer::start();
  while(!GB_Thermometer::updateStatistics()) { // Load temperature on startup
    delay(1000);
  }

  GB_Controller::checkFreeMemory();

  if(GB_SerialHelper::useSerialMonitor){ 
    printSendBootStatus(F("storage"));
    //GB_SerialHelper::printDirtyEnd();
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

  // Create main life circle timer
  Alarm.timerRepeat(UPDATE_THEMPERATURE_STATISTICS_DELAY, updateThermometerStatistics);  // repeat every N seconds
  Alarm.timerRepeat(UPDATE_WIFI_STATUS_DELAY, updateWiFiStatus);  
  Alarm.timerRepeat(UPDATE_GROWBOX_STATE_DELAY, updateGrowboxState);  // repeat every N seconds

  // Create suolemental rare switching
  Alarm.alarmRepeat(UP_HOUR, 00, 00, switchToDayMode);      // repeat once every day
  Alarm.alarmRepeat(DOWN_HOUR, 00, 00, switchToNightMode);  // repeat once every day

  if(GB_SerialHelper::useSerialMonitor){ 
    if (controllerFreeMemoryBeforeBoot != freeMemory()){
      printSendFreeMemory();
    }
    Serial.println(F("Growbox successfully started"));
    //GB_SerialHelper::printDirtyEnd();
  }

  if(GB_SerialHelper::useSerialMonitor){  
    GB_SerialHelper::printDirtyEnd();
  }

  if (GB_SerialHelper::useSerialWifi){ 
    GB_SerialHelper::setWifiConfiguration("Hell", "flat65router"); // OPTIMIZE IT
    GB_SerialHelper::startWifi();
  }

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

  boolean isWifiRequest;
  byte wifiPortDescriptor;

  String input; 
  if (!GB_SerialHelper::handleSerialEvent(input, g_isWifiRequest, g_wifiPortDescriptor)){
    return;
  }

  executeCommand(input);

  g_isWifiRequest = false;
  g_wifiPortDescriptor = 0x00;
}


/////////////////////////////////////////////////////////////////////
//                  TIMER/CLOCK EVENT HANDLERS                     //
/////////////////////////////////////////////////////////////////////

void updateGrowboxState() {

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

  updateGrowboxState();
}

void switchToNightMode(){
  if (g_isDayInGrowbox == false){
    return;
  }
  g_isDayInGrowbox = false;
  GB_Logger::logEvent(EVENT_MODE_NIGHT);

  updateGrowboxState();
}

/////////////////////////////////////////////////////////////////////
//                              SCHEDULE                           //
/////////////////////////////////////////////////////////////////////

void updateThermometerStatistics(){ // should return void
  GB_Thermometer::updateStatistics(); 
}

void updateWiFiStatus(){ // should return void
  GB_SerialHelper::updateWiFiStatus(); 
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

/////////////////////////////////////////////////////////////////////
//                     HTTP SUPPLEMENTAL COMMANDS                  //
/////////////////////////////////////////////////////////////////////

static void printSendData(const __FlashStringHelper* data){
  if (g_isWifiRequest){
    GB_SerialHelper::sendHTTPResponseData(g_wifiPortDescriptor, data);
  } 
  else {
    Serial.print(data); 
  }
}

static void printSendData(const String &data){
  if (g_isWifiRequest){
    GB_SerialHelper::sendHTTPResponseData(g_wifiPortDescriptor, data);
  } 
  else {
    Serial.print(data); 
  }
}

static void printSendDataLn(){
  printSendData(F("\r\n"));
}

/// TODO optimize it
static void printSendData(int data){
  String str; 
  str += data;
  printSendData(str);
}

static void printSendData(word data){
  String str; 
  str += data;
  printSendData(str);
}

static void printSendData(char data){
  String str; 
  str += data;
  printSendData(str);
}

static void printSendData(float data){
  String str = GB_PrintDirty::floatToString(data);
  printSendData(str);
}

static void printSendData(time_t data){
  String str = GB_PrintDirty::getTimeString(data);
  printSendData(str);
}

static void sendHTTPtagA(const __FlashStringHelper* url, const __FlashStringHelper* name){
  const __FlashStringHelper* part1 = F("<a href=\"");
  const __FlashStringHelper* part2 = F("\">");
  const __FlashStringHelper* part3 = F("</a> ");

  word length = flashStringLength(part1) + flashStringLength(part2) + flashStringLength(part3) + flashStringLength(url) + flashStringLength(name) ;

  GB_SerialHelper::sendHTTPResponseDataFrameStart(g_wifiPortDescriptor, length);
  Serial.print(part1);
  Serial.print(url);
  Serial.print(part2);
  Serial.print(name);
  Serial.print(part3);
  GB_SerialHelper::sendHTTPResponseDataFrameStop();
}

static void sendHTTPtagHR(){
  printSendData(F("<hr/>"));
}

static void executeCommand(String &input){

  if (g_isWifiRequest){
    GB_SerialHelper::sendHTTPResponseData(g_wifiPortDescriptor, F("<h1>Growbox</h1>"));
    sendHTTPtagA(F("/"), F("Home"));
    //sendHTTPtagA(F("/status"), F("Status"));
    sendHTTPtagA(F("/log"), F("Log"));
    sendHTTPtagHR();
  }

  if (input.equals("/")){
    printSendFullStatus(); 
  } 
  else if (input.equals("/log")){
    printSendFullLog(true, true, true); 
  }
  /*
  // read the incoming byte:
   char firstChar = 0, secondChar = 0; 
   firstChar = input[0];
   if (input.length() > 1){
   secondChar = input[1];
   }
   
   Serial.print(F("COMMAND>"));
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
   printprintSendFullStatus(); 
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
   */
  if (g_isWifiRequest) {
    GB_SerialHelper::finishHTTPResponse(g_wifiPortDescriptor);
  } 
  else { 
    // It was command from SerialMonotor 
    GB_SerialHelper::printDirtyEnd();
  }
}

static void printSendFullStatus(){
  GB_SerialHelper::sendHTTPResponseData(g_wifiPortDescriptor, F("<pre>"));
  
  printSendFreeMemory();
  printSendBootStatus();
  printSendTimeStatus();
  printSendTemperatureStatus();
  
  sendHTTPtagHR();
  
  printSendPinsStatus();
  
  GB_SerialHelper::sendHTTPResponseData(g_wifiPortDescriptor, F("</pre>"));
}

void printSendFreeMemory(){  
  printSendData(F("Free memory: "));
  printSendData(freeMemory()); 
  printSendData(F(" bytes\r\n"));
}

// private:

static void printSendBootStatus(){
  printSendData(F("Controller: frist startup time: ")); 
  printSendData(GB_StorageHelper::getFirstStartupTimeStamp());
  printSendData(F(", last startup time: ")); 
  printSendData(GB_StorageHelper::getLastStartupTimeStamp());
  printSendData(F("\r\nLogger: "));
  if (GB_StorageHelper::isStoreLogRecordsEnabled()){
    printSendData(F("enabled"));
  } 
  else {
    printSendData(F("disabled"));
  }
  printSendData(F(", records "));
  printSendData(GB_StorageHelper::getLogRecordsCount());
  printSendData('/');
  printSendData(GB_StorageHelper::LOG_CAPACITY);
  if (GB_StorageHelper::isLogOverflow()){
    printSendData(F(", overflow"));
  } 
  printSendDataLn();
}

static void printSendTimeStatus(){
  printSendData(F("Clock: ")); 
  if (g_isDayInGrowbox) {
    printSendData(F("DAY"));
  } 
  else{
    printSendData(F("NIGHT"));
  }
  printSendData(F(" mode, current time ")); 
  printSendData(now());
  printSendData(F(", up time [")); 
  printSendData(UP_HOUR); 
  printSendData(F(":00], down time ["));
  printSendData(DOWN_HOUR);
  printSendData(F(":00]\r\n"));
}

static void printSendTemperatureStatus(){
  float workingTemperature, statisticsTemperature;
  int statisticsCount;
  GB_Thermometer::getStatistics(workingTemperature, statisticsTemperature, statisticsCount);

  printSendData(F("Temperature: current ")); 
  printSendData(workingTemperature);
  printSendData(F(", next ")); 
  printSendData(statisticsTemperature);
  printSendData(F(" (count ")); 
  printSendData(statisticsCount);

  printSendData(F("), day "));
  printSendData(TEMPERATURE_DAY);
  printSendData(F("+/-"));
  printSendData(TEMPERATURE_DELTA);
  printSendData(F(", night "));
  printSendData(TEMPERATURE_NIGHT);
  printSendData(F("+/-"));
  printSendData(2*TEMPERATURE_DELTA);
  printSendData(F(", critical "));
  printSendData(TEMPERATURE_CRITICAL);
  printSendDataLn();
}

static void printSendPinsStatus(){
  printSendData(F("Pin OUTPUT INPUT")); 
  printSendDataLn();
  for(int i=0; i<=19;i++){
    printSendData(' ');
    if (i>=14){
      printSendData('A');
      printSendData(i-14);
    } 
    else { 
      printSendData(GB_PrintDirty::getFixedDigitsString(i, 2));
    }
    printSendData(F("  ")); 

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
      printSendData(F("  "));
      printSendData(dataStatus);
      printSendData(F("     -   "));
    } 
    else {
      printSendData(F("  -     "));
      printSendData(inputStatus);
      printSendData(F("   "));
    }

#ifdef GB_EXTRA_STRINGS
    switch(i){
    case 0: 
    case 1: 
      printSendData(F("Reserved by Serial/USB. Can be used, if Serial/USB won't be connected"));
      break;
    case LIGHT_PIN: 
      printSendData(F("Relay: light on(0)/off(1)"));
      break;
    case FAN_PIN: 
      printSendData(F("Relay: fun on(0)/off(1)"));
      break;
    case FAN_SPEED_PIN: 
      printSendData(F("Relay: fun max(0)/min(1) speed switch"));
      break;
    case ONE_WIRE_PIN: 
      printSendData(F("1-Wire: termometer"));
      break;
    case USE_SERIAL_MONOTOR_PIN: 
      printSendData(F("Use serial monitor on(1)/off(0)"));
      break;
    case ERROR_PIN: 
      printSendData(F("Error status"));
      break;
    case BREEZE_PIN: 
      printSendData(F("Breeze"));
      break;
    case 18: 
    case 19: 
      printSendData(F("Reserved by I2C. Can be used, if SCL, SDA pins will be used"));
      break;
    }
#endif
    printSendDataLn();
  }
}


static void printSendFullLog(boolean printEvents, boolean printErrors, boolean printTemperature){
  LogRecord logRecord;
  boolean isEmpty = true;
  printSendData(F("<table>"));
  for (int i = 0; i<=GB_Logger::getLogRecordsCount(); i++){
  
    logRecord = GB_Logger::getLogRecordByIndex(i);
    if (!printEvents && GB_Logger::isEvent(logRecord)){
      continue;
    }
    if (!printErrors && GB_Logger::isError(logRecord)){
      continue;
    }
    if (!printTemperature && GB_Logger::isTemperature(logRecord)){
      continue;
    }
    
    printSendData(F("<tr>"));
    printSendData(F("<td>"));
    printSendData(i+1);
    printSendData(F("</td><td>"));
    printSendData(GB_PrintDirty::getTimeString(logRecord.timeStamp));    
    printSendData(F("</td><td>"));
    printSendData(GB_PrintDirty::getHEX(logRecord.data, true));
    printSendData(F("</td><td>"));
    printSendData(GB_Logger::getLogRecordDescription(logRecord));
    printSendData(GB_Logger::getLogRecordSuffix(logRecord));
    printSendData(F("</td>"));
    //printSendDataLn();
    printSendData(F("</tr>"));
    isEmpty = false;

  }
  printSendData(F("</table>"));
  if (isEmpty){
    printSendData(F("Log empty"));
  }
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
      GB_PrintDirty::printHEX(i/16);
    }
    Serial.print(" ");
    GB_PrintDirty::printHEX(value);
    Serial.print(" ");
  }
  Serial.println();  
}



