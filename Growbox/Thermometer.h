#ifndef Thermometer_h
#define Thermometer_h

// Termometer
#include <DallasTemperature.h>
#include "Logger.h"

class ThermometerClass{

private:
  // Pass our oneWire reference to Dallas Temperature. 
  static DallasTemperature c_dallasTemperature;
  static DeviceAddress c_oneWireAddress;

  static float c_workingTemperature;
  static double c_statisticsTemperatureSumm;
  static int c_statisticsTemperatureCount;

public:

  static void init();
  
  static boolean updateStatistics();
  
  static float getTemperature(boolean forceLog = false);

  static void getStatistics(float &_workingTemperature, float &_statisticsTemperature, int &_statisticsTemperatureCount);

};

extern ThermometerClass GB_Thermometer;
#endif






