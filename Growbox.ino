#include <MemoryFree.h>;

#include <Time.h>
#include <TimeAlarms.h>

// RTC
#include <Wire.h>  
#include <DS1307RTC.h>

// Termometer
#include <OneWire.h>
#include <DallasTemperature.h>

#define OFFSETOF(type, field)    ((unsigned long) &(((type *) 0)->field))

/////////////////////////////////////////////////////////////////////
//                     HARDWARE CONFIGURATION                      //
/////////////////////////////////////////////////////////////////////

// relay pins
const byte LIGHT_PIN = 3;
const byte FAN_PIN = 4;
const byte FAN_SPEED_PIN = 5;

// 1-Wire pins
const byte ONE_WIRE_PIN = 8;

// status pins
const byte USE_SERIAL_MONOTOR_PIN = 11;
const byte ERROR_PIN = 12;
const byte BREEZE_PIN = LED_BUILTIN; //13

/////////////////////////////////////////////////////////////////////
//                     SOFTWARE CONFIGURATION                      //
/////////////////////////////////////////////////////////////////////

// Light scheduler
const byte UP_HOUR = 1;
const byte DOWN_HOUR = 9;  // 16/8

// Minimum Growbox reaction time
const int MAIN_LOOP_DELAY = 1; // 1 sec
const int CHECK_TEMPERATURE_DELAY = 20; //20 sec 
const int CHECK_GROWBOX_DELAY = 5*60; // 5 min 
const int WIFI_DELAY = 250; // 250 ms, delay after "at+" commands 

const float TEMPERATURE_DAY = 26.0;  //23-25, someone 27 
const float TEMPERATURE_NIGHT = 22.0;  // 18-22, someone 22
const float TEMPERATURE_CRITICAL = 35.0; // 35 max, 40 die
const float TEMPERATURE_DELTA = 3.0;   // +/-(3-4) Ok

// error blinks in milleseconds and blink sequences
const word ERROR_SHORT_SIGNAL_MS = 100;  // -> 0
const word ERROR_LONG_SIGNAL_MS = 400;   // -> 1
const word ERROR_DELAY_BETWEEN_SIGNALS_MS = 150;

// Wi-Fi
const String WIFI_WELLCOME = "Welcome to RAK410\r\n";
String WIFI_SID = "Hell";
String WIFI_PASS = "flat65router";

/////////////////////////////////////////////////////////////////////
//                         DEVICE CONSTANTCS                       //
/////////////////////////////////////////////////////////////////////

// Relay consts
const byte RELAY_ON = LOW, RELAY_OFF = HIGH;

// Serial consts
const byte SERIAL_ON = HIGH, SERIAL_OFF = LOW;

// Fun speed
const byte FAN_SPEED_MIN = RELAY_OFF;
const byte FAN_SPEED_MAX = RELAY_ON;

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

float g_temperature = 0.0;
double g_temperatureSumm = 0.0;
int g_temperatureSummCount = 0;

byte g_isDayInGrowbox = -1;
volatile boolean g_UseSerial = false;
volatile boolean g_UseSerialMonitor = false;
volatile boolean g_UseSerialWifi = false;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWirePin(ONE_WIRE_PIN);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature g_dallasTemperature(&oneWirePin);
DeviceAddress g_thermometerOneWireAddress;

/////////////////////////////////////////////////////////////////////
//                            STORAGE                              //
/////////////////////////////////////////////////////////////////////

const byte LOG_RECORD_SIZE = 5;
const byte BOOT_RECORD_SIZE = 32;
const word MAGIC_NUMBER = 0xAA55;

class StorageClass {
private:
  static const int AT24C32 = 0x50; // External EEPROM I2C address
public:
  static const word capacity = 0x1000; // 4K byte = 32K bit

  boolean isPresent(void) {     // check if the device is present
    Wire.beginTransmission(AT24C32);
    if (Wire.endTransmission() == 0)
      return true;
    return false;
  }

  void write(const word address, const byte data) {
    if (address >= capacity){
      return;
    }
    Wire.beginTransmission(AT24C32);
    Wire.write((byte)(address >> 8)); // MSB
    Wire.write((byte)(address & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();  
    delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
  }

  byte read(word address) {
    Wire.beginTransmission(AT24C32);
    Wire.write((byte)(address >> 8)); // MSB
    Wire.write((byte)(address & 0xFF)); // LSB
    Wire.endTransmission();
    delay(10);  // http://www.hobbytronics.co.uk/arduino-external-eeprom
    Wire.requestFrom(AT24C32, 1);
    if (Wire.available()) {
      return Wire.read();
    } 
    else {
      return 0xFF;
    }
  }

  void write(word address, const void* data, const byte sizeofData) {
    for (word c = 0; c < sizeofData; c++){
      byte value = ((byte*)data)[c];
      write(address + c, value);
    }
  }

  void read(word address, void *data, const byte sizeofData) {
    for (word c = 0; c < sizeofData; c++){
      byte value =  read(address + c);
      ((byte*)data)[c] = value;
    }
  }
} 
STORAGE;

struct LogRecord {
  time_t timeStamp;
  byte data;  

  LogRecord (byte data): 
  timeStamp(now()), data(data) {
  }
  LogRecord (){
  }
};

struct BootRecord { // 32 bytes
  word first_magic;                 //  2
  time_t firstStartupTimeStamp;     //  4
  time_t lastStartupTimeStamp;      //  4
  word nextLogRecordAddress;        //  2
  struct  {
boolean isLogOverflow :
    1;  
boolean isLoggerEnabled :
    2;     
  } 
  boolPreferencies;                 //  1 
  byte reserved[17];                //  

  word last_magic;                  //  2

  boolean initOnStartup(){
    STORAGE.read(0, this, sizeof(BootRecord));
    boolean isRestart = isCorrect();
    if (isRestart){
      lastStartupTimeStamp = now();
      STORAGE.write(OFFSETOF(BootRecord, lastStartupTimeStamp), &(lastStartupTimeStamp), sizeof(lastStartupTimeStamp));   
    } 
    else {
      initAtFirstStartUp();
    }
    return isRestart;
  }

  void cleanupLog(){
    nextLogRecordAddress = sizeof(BootRecord);
    STORAGE.write(OFFSETOF(BootRecord, nextLogRecordAddress), &(nextLogRecordAddress), sizeof(nextLogRecordAddress)); 
    boolPreferencies.isLogOverflow = false;
    STORAGE.write(OFFSETOF(BootRecord, boolPreferencies), &(boolPreferencies), sizeof(boolPreferencies));
  }

  void increaseNextLogRecordAddress(){
    nextLogRecordAddress += sizeof(LogRecord);  
    if (nextLogRecordAddress+sizeof(LogRecord) >= STORAGE.capacity){
      nextLogRecordAddress = sizeof(BootRecord);
      if (!boolPreferencies.isLogOverflow){
        boolPreferencies.isLogOverflow = true;
        STORAGE.write(OFFSETOF(BootRecord, boolPreferencies), &(boolPreferencies), sizeof(boolPreferencies)); 
      }
    }
    STORAGE.write(OFFSETOF(BootRecord, nextLogRecordAddress), &(nextLogRecordAddress), sizeof(nextLogRecordAddress)); 
  }

  void setLoggerEnable(boolean flag){
    boolPreferencies.isLoggerEnabled = flag;
    STORAGE.write(OFFSETOF(BootRecord, boolPreferencies), &(boolPreferencies), sizeof(boolPreferencies)); 

  }

  boolean isCorrect(){
    return (first_magic == MAGIC_NUMBER) && (last_magic == MAGIC_NUMBER);
  }
private:
  void initAtFirstStartUp(){
    first_magic = MAGIC_NUMBER;
    firstStartupTimeStamp = now();
    lastStartupTimeStamp = firstStartupTimeStamp;
    nextLogRecordAddress = sizeof(BootRecord);
    boolPreferencies.isLogOverflow = false;
    boolPreferencies.isLoggerEnabled = true;
    for(byte i=0; i<sizeof(reserved); i++){
      reserved[i] = 0;
    }
    last_magic = MAGIC_NUMBER;
    STORAGE.write(0, this, sizeof(BootRecord));
  }
} 
BOOT_RECORD;

/////////////////////////////////////////////////////////////////////
//                              LOGGING                            //
/////////////////////////////////////////////////////////////////////

class Error {
private:
  static Error* lastAddedItem;
  const Error* nextError;

public:
  // sequence - human readble sequence of signals, e.g.[B010] means [short, long, short] 
  // sequenceSize - byte in region 1..4
  byte sequence; 
  byte sequenceSize;
  const __FlashStringHelper* description; // FLASH
  boolean isStored; // should be stored in Log only once, but notification should repeated

    Error() : 
  nextError(lastAddedItem), sequence(0xFF), sequenceSize(0xFF), isStored(false) {
    lastAddedItem = this;
  }

  void init(byte sequence, byte sequenceSize, const __FlashStringHelper* description) {
    this->sequence = sequence;
    this->sequenceSize = sequenceSize;
    this->description = description;
  }

  void notify() {
    digitalWrite(ERROR_PIN, LOW);
    delay(1000);
    for (int i = sequenceSize-1; i >= 0; i--){
      digitalWrite(ERROR_PIN, HIGH);
      if (bitRead(sequence, i)){
        delay(ERROR_LONG_SIGNAL_MS);
      } 
      else {
        delay(ERROR_SHORT_SIGNAL_MS);
      } 
      digitalWrite(ERROR_PIN, LOW);
      delay(ERROR_DELAY_BETWEEN_SIGNALS_MS);
    }
    digitalWrite(ERROR_PIN, LOW);
    delay(1000);
  }

  static Error* findByIndex(byte sequence, byte sequenceSize){
    Error* currentItemPtr = lastAddedItem;
    while (currentItemPtr != 0){
      if (currentItemPtr->sequence == sequence && currentItemPtr->sequenceSize == sequenceSize) {
        break;
      }
      currentItemPtr = (Error*)currentItemPtr->nextError;
    }
    return 0;
  }

  static boolean isInitialized(){
    return (findByIndex(0xFF, 0xFF) == 0);
  }

};
Error* Error::lastAddedItem = 0;

Error 
ERROR_TIMER_NOT_SET,
ERROR_TIMER_NEEDS_SYNC,
ERROR_TERMOMETER_DISCONNECTED,
ERROR_TERMOMETER_ZERO_VALUE,
ERROR_TERMOMETER_CRITICAL_VALUE,
ERROR_MEMORY_LOW;

void initErrors(){
  // Use F macro to reduce requirements to memory. We can't use F macro in constructors.
  ERROR_TIMER_NOT_SET.init(B00, 2, F("Error: Timer not set"));
  ERROR_TIMER_NEEDS_SYNC.init(B001, 3, F("Error: Timer needs sync"));
  ERROR_TERMOMETER_DISCONNECTED.init(B01, 2, F("Error: Termometer disconnected"));
  ERROR_TERMOMETER_ZERO_VALUE.init(B010, 3, F("Error: Termometer returned ZERO value"));
  ERROR_TERMOMETER_CRITICAL_VALUE.init(B000, 3, F("Error: Termometer returned CRITICAL value"));
  ERROR_MEMORY_LOW.init(B111, 3, F("Error: Memory remained less 200 bytes"));
}

class Event {
private:
  static Event* lastAddedEvent;
  const Event* nextEvent;

public:
  byte index;
  const __FlashStringHelper* description; // FLASH

  Event() : 
  nextEvent(lastAddedEvent), index(0xFF) {
    lastAddedEvent = this;
  }

  void init(byte index, const __FlashStringHelper* description) {
    this->index = index;
    this->description = description;
  }

  static Event* findByIndex(byte index){
    Event* currentItemPtr = lastAddedEvent;
    while (currentItemPtr != 0){
      if (currentItemPtr->index == index) {
        break;
      }
      currentItemPtr = (Event*)currentItemPtr->nextEvent;
    }
    return 0;
  }

  static boolean isInitialized(){
    return (findByIndex(0xFF) == 0);
  }

};
Event* Event::lastAddedEvent = 0;

/*EVENT_DATA_CHECK_PERIOD_SEC,
 EVENT_DATA_TEMPERATURE_DAY,
 EVENT_DATA_TEMPERATURE_NIGHT,
 EVENT_DATA_TEMPERATURE_CRITICAL,
 EVENT_DATA_TEMPERATURE_DELTA,*/
//EVENT_RTC_SYNKC(10, "RTC synced");

Event 
EVENT_FIRST_START_UP, 
EVENT_RESTART, 
EVENT_MODE_DAY, 
EVENT_MODE_NIGHT, 
EVENT_LIGHT_OFF, 
EVENT_LIGHT_ON, 
EVENT_FAN_OFF, 
EVENT_FAN_ON_MIN, 
EVENT_FAN_ON_MAX,
EVENT_SERIAL_UNKNOWN_COMMAND;

void initEvents(){
  // Use F macro to reduce requirements to memory. We can't use F macro in constructors.
  EVENT_FIRST_START_UP.init(1, F("FIRST STARTUP")), 
  EVENT_RESTART.init(2, F("RESTARTED")), 
  EVENT_MODE_DAY.init(3, F("Growbox switched to DAY mode")), 
  EVENT_MODE_NIGHT.init(4, F("Growbox switched to NIGHT mode")), 
  EVENT_LIGHT_OFF.init(5, F("LIGHT turned OFF")), 
  EVENT_LIGHT_ON.init(6, F("LIGHT turned ON")), 
  EVENT_FAN_OFF.init(7, F("FAN turned OFF")), 
  EVENT_FAN_ON_MIN.init(8, F("FAN turned ON MIN speed")), 
  EVENT_FAN_ON_MAX.init(9, F("FAN turned ON MAX speed")),
  EVENT_SERIAL_UNKNOWN_COMMAND.init(10, F("Unknown serial command"));
}

class LoggerClass {
public:
  // Normal event uses uses format [00DDDDDD]
  //   00 - prefix for normal events 
  //   DDDDDD - event identificator
  void logEvent(Event &event){
    LogRecord logRecord(event.index);
    logRawData(logRecord, event.description, true);
  }

  // Error events uses format [01SSDDDD] 
  //   01 - prefix for error events 
  //   SS - length of errir seqence 
  //   DDDD - sequence data
  void logError(Error &error){
    LogRecord logRecord(B01000000|(B00000011 | error.sequenceSize-1)<<4 | (B00001111 & error.sequence));

    logRawData(logRecord, error.description, !error.isStored);
    error.isStored = true;   
    error.notify();
  }
  boolean stopLogError(Error &error){
    if (error.isStored){
      error.isStored = false;
      return true;
    }
    return false;
  }

  // Termometer events uses format [11TTTTTT].
  //   11 - prefix for termometer events
  //   TTTTTT - temperature [0..2^6] = [0..64]
  void logTemperature(byte temperature){
    LogRecord logRecord(B11000000|temperature);
    logRawData(logRecord, F("Temperature"), true, temperature);
  }

  void printLogRecord(LogRecord &logRecord) {

    byte data = (logRecord.data & B00111111);

    if (isEvent(logRecord)){
      Event* foundItemPtr = Event::findByIndex(data);
      if (foundItemPtr == 0){
        logRawData(logRecord, F("Unknown event"), false);
      } 
      else {
        logRawData(logRecord, foundItemPtr->description, false);
      }

    } 
    else if (isTemperature(logRecord)){
      logRawData(logRecord, F("Temperature"), false, data); 

    } 
    else if (isError(logRecord)){    
      byte sequence = (data & B00001111); 
      byte sequenceSize = (data & B00110000)>>4; 
      Error* foundItemPtr = Error::findByIndex(sequence, sequenceSize);
      if (foundItemPtr == 0){
        logRawData(logRecord, F("Unknown error"), false);
      } 
      else {
        logRawData(logRecord, foundItemPtr->description, false);
      }
    } 
    else {
      logRawData(logRecord, F("Unknown"), false);
    }
  }

  boolean isEvent(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B00000000;
  }
  boolean isError(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B01000000;
  }
  boolean isTemperature(LogRecord &logRecord){
    return (logRecord.data & B11000000) == B11000000;
  }

private:

  void logRawData(const LogRecord &logRecord, const __FlashStringHelper* description, boolean storeLog, byte temperature = 0){

    storeLog = storeLog && BOOT_RECORD.isCorrect() && BOOT_RECORD.boolPreferencies.isLoggerEnabled && STORAGE.isPresent();

    if (storeLog) {
      STORAGE.write(BOOT_RECORD.nextLogRecordAddress, &logRecord, sizeof(LogRecord));
      BOOT_RECORD.increaseNextLogRecordAddress();
    }

    if (g_UseSerialMonitor) {
      if (storeLog == false) {
        Serial.print(F("NOT STORED "));
      }
      printTime(logRecord.timeStamp); 
      Serial.print(F(" 0x")); 
      print2digitsHEX(logRecord.data);
      Serial.print(' '); 
      Serial.print(description);
      if (temperature !=0) {
        Serial.print(F(" ["));
        Serial.print((unsigned byte)temperature);
        Serial.print(F("] C"));
      }
      Serial.println();
      printEnd();
    }
  }
} 
LOGGER;

/////////////////////////////////////////////////////////////////////
//                            CONTROLLER                           //
/////////////////////////////////////////////////////////////////////

void rebootController() {
  if (g_UseSerialMonitor) {
    Serial.println(F("Rebooting..."));
  }
  void(* resetFunc) (void) = 0; // Reset MC function
  resetFunc(); //call
}


/////////////////////////////////////////////////////////////////////
//                              STATUS                             //
/////////////////////////////////////////////////////////////////////

boolean isDayInGrowbox(){
  if(timeStatus() == timeNeedsSync){
    LOGGER.logError(ERROR_TIMER_NEEDS_SYNC);
  } 
  else {
    LOGGER.stopLogError(ERROR_TIMER_NEEDS_SYNC);
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
    LOGGER.logError(ERROR_MEMORY_LOW);   
  } 
  else {
    LOGGER.stopLogError(ERROR_MEMORY_LOW); 
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
    LOGGER.logError(ERROR_TIMER_NOT_SET);
    setSyncProvider(RTC.get); // try to refresh clock
  }
  LOGGER.stopLogError(ERROR_TIMER_NOT_SET); 

  checkFreeMemory();

  if(g_UseSerialMonitor){ 
    Serial.println(F("Checking termometer"));
    printEnd();
  }

  // Configure termometer
  g_dallasTemperature.begin();
  while(g_dallasTemperature.getDeviceCount() == 0){
    LOGGER.logError(ERROR_TERMOMETER_DISCONNECTED);
    g_dallasTemperature.begin();
  }  
  LOGGER.stopLogError(ERROR_TERMOMETER_DISCONNECTED);

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
    LOGGER.logEvent(EVENT_RESTART);
  } 
  else {
    LOGGER.logEvent(EVENT_FIRST_START_UP);
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
    
  } else if (g_UseSerialMonitor) {
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
    LOGGER.logError(ERROR_TERMOMETER_CRITICAL_VALUE);
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
  LOGGER.logEvent(EVENT_MODE_DAY);

  checkGrowboxState();
}

void switchToNightMode(){
  if (g_isDayInGrowbox == false){
    return;
  }
  g_isDayInGrowbox = false;
  LOGGER.logEvent(EVENT_MODE_NIGHT);

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
    LOGGER.logError(ERROR_TERMOMETER_DISCONNECTED);
    return false;
  };

  float freshTemperature = g_dallasTemperature.getTempC(g_thermometerOneWireAddress);

  if ((int)freshTemperature == 0){
    LOGGER.logError(ERROR_TERMOMETER_ZERO_VALUE);  
    return false;
  }

  g_temperatureSumm += freshTemperature;
  g_temperatureSummCount++;

  boolean forceLog = 
    LOGGER.stopLogError(ERROR_TERMOMETER_ZERO_VALUE) |
    LOGGER.stopLogError(ERROR_TERMOMETER_DISCONNECTED); 
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
    LOGGER.logTemperature((byte)freshTemperature);
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
  LOGGER.logEvent(EVENT_LIGHT_ON);
}

void turnOffLight(){
  if (digitalRead(LIGHT_PIN) == RELAY_OFF){
    return;
  }
  digitalWrite(LIGHT_PIN, RELAY_OFF);  
  LOGGER.logEvent(EVENT_LIGHT_OFF);
}

void turnOnFan(int speed){
  if (digitalRead(FAN_PIN) == RELAY_ON && digitalRead(FAN_SPEED_PIN) == speed){
    return;
  }
  digitalWrite(FAN_SPEED_PIN, speed);
  digitalWrite(FAN_PIN, RELAY_ON);

  if (speed == FAN_SPEED_MIN){
    LOGGER.logEvent(EVENT_FAN_ON_MIN);
  } 
  else {
    LOGGER.logEvent(EVENT_FAN_ON_MAX);
  }
}

void turnOffFan(){
  if (digitalRead(FAN_PIN) == RELAY_OFF){
    return;
  }
  digitalWrite(FAN_PIN, RELAY_OFF);
  digitalWrite(FAN_SPEED_PIN, RELAY_OFF);
  LOGGER.logEvent(EVENT_FAN_OFF);
}

/////////////////////////////////////////////////////////////////////
//                              PRINT                              //
/////////////////////////////////////////////////////////////////////

void printEnd(){
  if (g_UseSerialWifi) {
    delay(250);
    while (Serial.available()){
      Serial.read();
    }
  }
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
