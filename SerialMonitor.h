#ifndef GB_SerialMonitor_h
#define GB_SerialMonitor_h

#include <Time.h>

#include "Global.h"
#include "Storage.h"
#include "LogRecord.h"

void printEnd();

void printLogRecord(word address, word index, boolean events, boolean errors, boolean temperature);
void printFullLog(boolean events, boolean errors, boolean temperature);


void printStatus();

void printFreeMemory();

void printBootStatus();
void printTimeStatus();

void printTemperatureStatus();
void printPinsStatus();

void printTime(time_t time);

void print2digits(byte digits);

void print2digitsHEX(byte digits);

void printRAM(void *ptr, byte sizeOf);

void printStorage(word address, byte sizeOf);

void printStorage();

void fillStorage(byte value);

void checkSerialCommands();


#endif




