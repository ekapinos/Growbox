#ifndef Thermometer_h
#define Thermometer_h

// Termometer
#include <DallasTemperature.h>
#include "Logger.h"

class ThermometerClass{
private:
  // Pass our oneWire reference to Dallas Temperature. 
  DallasTemperature c_dallasTemperature;

  float c_lastTemperature;
  double c_statisticsTemperatureSumm;
  int c_statisticsTemperatureCount;

public:
  ThermometerClass(OneWire*);
private:
  float getHardwareTemperature(boolean logOnError);

public:
  boolean isPresent();

  void setUseThermometer(boolean flag);
  boolean isUseThermometer();

  void updateStatistics();
  float getTemperature(); // may be NAN
  void getStatistics(float &_workingTemperature, float &_statisticsTemperature, int &_statisticsTemperatureCount);

};

extern ThermometerClass GB_Thermometer;
#endif

