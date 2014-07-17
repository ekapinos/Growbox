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
  boolPreferencies;                 //  1 
  byte reserved[145];               //  <----reserved
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



