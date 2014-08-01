#ifndef StorageModel_h
#define StorageModel_h

#include <Time.h>
#include "Global.h"

const word LOG_RECORD_SIZE = 0x5;
const word BOOT_RECORD_SIZE = 0x100; // 256

const word MAGIC_NUMBER = 0xAA55;   //  2
const byte WIFI_SSID_LENGTH = 0x20; // 32
const byte WIFI_PASS_LENGTH = 0x40; // 64

struct BootRecord {
  word first_magic;                 //  2
  time_t firstStartupTimeStamp;     //  4
  time_t lastStartupTimeStamp;      //  4
  word nextLogRecordIndex;          //  2
  struct BoolPreferencies {
boolean isLogOverflow :
    1;  
boolean isLoggerEnabled :
    2;      
boolean isWifiStationMode :
// false - Access point, true - Station mode
    3;    
  } 
  boolPreferencies;                 // 1
  
  word turnToDayModeAt;             // 2
  word turnToNightModeAt;           // 2
  byte normalTemperatueDayMin;      // 1
  byte normalTemperatueDayMax;      // 1
  byte normalTemperatueNightMin;    // 1
  byte normalTemperatueNightMax;    // 1
  byte criticalTemperatue;          // 1
  
  byte wateringSystemCount;         // 1  [0..MAX_WATERING_SYSTEMS_COUNT]
  struct WateringSystemPreferencies {
    
    struct WateringSystemBoolPreferencies{
      boolean isSensorConnected : 1; 
      boolean isWaterPumpConnected : 2; 
    } boolPreferencies;     // 1
    
    byte notConnectedValue; // 1 (veryDryValue..airValue] (airValue..1023]  (t >> 2) (10 bit to 8)
    byte veryDryValue;      // 1 (dryValue..veryDryValue]
    byte dryValue;          // 1 (normalValue..dryValue]
    byte normalValue;       // 1 (wetValue..normalValue]
    byte wetValue;          // 1 (veryWetValue..wetValue]
    byte veryWetValue;      // 1 (shortСircuitValue..veryWetValue]
    byte shortCircitValue;  // 1 [0..shortСircuitValue]
    
    byte dryWateringDuration;     // 1 // seconds
    byte veryDryWateringDuration; // 1 // seconds
    
    word wateringIfNoSensorAt;    // 2 // minutes from midnight, dryWateringDuration used
    
  } wateringSystemPreferencies[MAX_WATERING_SYSTEMS_COUNT]; // 12*MAX_WATERING_SYSTEMS_COUNT(4) = 48
  
  byte reserved[87];                //  <----reserved
  char wifiSSID[WIFI_SSID_LENGTH];  // 32  
  char wifiPass[WIFI_PASS_LENGTH];  // 64
  word last_magic;                  //  2  
};

struct LogRecord {
  time_t timeStamp;                 // 4
  byte data;                        // 1

  LogRecord (byte data): 
  timeStamp(now()), data(data) {
  }

  LogRecord (){
  }
};

#endif



