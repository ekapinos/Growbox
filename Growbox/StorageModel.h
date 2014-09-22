#ifndef StorageModel_h
#define StorageModel_h

#include <Time.h>
#include "Global.h"

const word BOOT_RECORD_SIZE = 0x100; // 256

const word MAGIC_NUMBER = 0xAA55;   //  2
const byte WIFI_SSID_LENGTH = 0x20; // 32
const byte WIFI_PASS_LENGTH = 0x40; // 64

struct BootRecord{
  word first_magic;                 //  2
  time_t firstStartupTimeStamp;     //  4
  time_t startupTimeStamp;          //  4
  word nextLogRecordIndex;          //  2
  struct BoolPreferencies{
    boolean isLogOverflow :1;
    boolean isLoggerEnabled :1;
    boolean isWifiStationMode :1;   // false - Access point, true - Station mode
    boolean isAutoCalculatedClockTimeUsed :1;
    boolean useExternal_EEPROM_AT24C32 :1;
    boolean useThermometer :1;
    boolean useRTC :1;
    boolean useFan :1;

    boolean useLight :1;
    boolean useHeater :1;
  } boolPreferencies;                 // 2

  word turnToDayModeAt;             // 2
  word turnToNightModeAt;           // 2
  byte normalTemperatureDayMin;      // 1
  byte normalTemperatureDayMax;      // 1
  byte normalTemperatureNightMin;    // 1
  byte normalTemperatureNightMax;    // 1
  byte criticalTemperatureMax;       // 1

  struct WateringSystemPreferencies{

    struct WateringSystemBoolPreferencies{
      boolean isWetSensorConnected :1;
      boolean isWaterPumpConnected :1;
      boolean useWetSensorForWatering :1;
      boolean skipNextWatering :1; // used on date/time change
    } boolPreferencies;     // 1

    byte inAirValue; // 1 (inAirValue..1022], [1023] - Not connected  (t >> 2) (10 bit to 8)
    byte veryDryValue;      // 1 (veryDryValue..notConnectedValue]
    byte dryValue;          // 1 (dryValue..veryDryValue]
    byte normalValue;       // 1 (normalValue..dryValue]
    byte wetValue;          // 1 (wetValue..normalValue]
    byte veryWetValue; // 1 (veryWetValue..wetValue] [0..veryWetValue] - short circit

    byte dryWateringDuration;     // 1 // seconds
    byte veryDryWateringDuration; // 1 // seconds

    word startWateringAt; // 2 // minutes from midnight, dryWateringDuration used

    time_t lastWateringTimeStamp; // 4 

  } wateringSystemPreferencies[MAX_WATERING_SYSTEMS_COUNT]; // 16*MAX_WATERING_SYSTEMS_COUNT(4) = 64

  int16_t autoAdjustClockTimeDelta; // 2
  byte criticalTemperatureMin;      // 1 // TODO move upper

  byte reserved[70];                //  <----reserved
  char wifiSSID[WIFI_SSID_LENGTH];  // 32  
  char wifiPass[WIFI_PASS_LENGTH];  // 64
  word last_magic;                  //  2  
};

struct LogRecord{
  time_t timeStamp;                 // 4
  byte data;                        // 1
  byte data1;                       // 1

  LogRecord(byte data) :
      timeStamp(now()), data(data), data1(0) {
  }
  LogRecord(byte data, byte data1) :
      timeStamp(now()), data(data), data1(data1) {
  }

  LogRecord() :
      timeStamp(0), data(0), data1(0) {
  }

  boolean isEmpty() const {
    return (timeStamp == 0 && data == 0 && data1 == 0);
  }
};

#endif

