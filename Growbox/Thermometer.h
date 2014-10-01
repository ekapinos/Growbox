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
  float getHardwareTemperature(boolean logOnError = false);

  boolean isPresent();

  void setUseThermometer(boolean flag);
  boolean isUseThermometer();

  void updateStatistics();
  float getTemperatureAndClearStatistics(); // may be NAN

  float getLastTemperature();
  float getForecastTemperature();
  int getForecastMeasurementCount();

  /////////////////////////////////////////////////////////////////////
  //                              OTHER                              //
  /////////////////////////////////////////////////////////////////////
private:
  template<class T>
    void showMessage(T str, boolean newLine = true) {
      if (g_useSerialMonitor) {
        Serial.print(F("THERMOMETER> "));
        Serial.print(str);
        if (newLine) {
          Serial.println();
        }
      }
    }
};

extern ThermometerClass GB_Thermometer;
#endif

