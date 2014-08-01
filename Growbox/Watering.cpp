#include "Watering.h"

byte WateringClass::analogToByte(word input){
  return (input>>2); // 10 bit in analog input
}

WateringClass GB_Watering;
