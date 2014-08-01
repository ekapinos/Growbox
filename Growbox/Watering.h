#ifndef Watering_h
#define Watering_h

#include "Global.h"

class WateringClass{
  word lastValue[MAX_WATERING_SYSTEMS_COUNT];
public:
  byte analogToByte(word input);
};

extern WateringClass GB_Watering;

#endif
