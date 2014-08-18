#include "Thermometer.h"

#include "Logger.h"

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature ThermometerClass::c_dallasTemperature(&g_oneWirePin);
DeviceAddress ThermometerClass::c_oneWireAddress;

// Visible only in this file
float ThermometerClass::c_workingTemperature = 0.0;
double ThermometerClass::c_statisticsTemperatureSumm = 0.0;
int ThermometerClass::c_statisticsTemperatureCount = 0;


void ThermometerClass::init(){
  c_dallasTemperature.begin();
  while(c_dallasTemperature.getDeviceCount() == 0){
    GB_Logger.logError(ERROR_TERMOMETER_DISCONNECTED);
    c_dallasTemperature.begin();
  }  
  GB_Logger.stopLogError(ERROR_TERMOMETER_DISCONNECTED);

  c_dallasTemperature.getAddress(c_oneWireAddress, 0); // search for devices on the bus and assign based on an index.
}

boolean ThermometerClass::updateStatistics(){

  if(!c_dallasTemperature.requestTemperaturesByAddress(c_oneWireAddress)){
    GB_Logger.logError(ERROR_TERMOMETER_DISCONNECTED);
    return false;
  };

  float freshTemperature = c_dallasTemperature.getTempC(c_oneWireAddress);

  if ((int)freshTemperature == 0){
    GB_Logger.logError(ERROR_TERMOMETER_ZERO_VALUE);  
    return false;
  }

  c_statisticsTemperatureSumm += freshTemperature;
  c_statisticsTemperatureCount++;

  boolean forceLog = 
    GB_Logger.stopLogError(ERROR_TERMOMETER_ZERO_VALUE) |
    GB_Logger.stopLogError(ERROR_TERMOMETER_DISCONNECTED); 
  if (forceLog) {
    getTemperature(true);
  }
  else if (c_statisticsTemperatureCount > 50){
    getTemperature(); // prevents overflow 
  }

  return true;
}

float ThermometerClass::getTemperature(boolean forceLog){

  if (c_statisticsTemperatureCount == 0){
    return c_workingTemperature; 
  }

  float freshTemperature = c_statisticsTemperatureSumm/c_statisticsTemperatureCount;

  if (((int)freshTemperature != (int)c_workingTemperature) || forceLog) {          
    GB_Logger.logTemperature((byte)freshTemperature);
  }

  c_workingTemperature = freshTemperature;

  c_statisticsTemperatureSumm = 0.0;
  c_statisticsTemperatureCount = 0;

  return c_workingTemperature;
}

void ThermometerClass::getStatistics(float &_workingTemperature, float &_statisticsTemperature, int &_statisticsTemperatureCount){
  _workingTemperature = c_workingTemperature;

  if (c_statisticsTemperatureCount != 0){
    _statisticsTemperature = c_statisticsTemperatureSumm/c_statisticsTemperatureCount;
  } 
  else {
    _statisticsTemperature = c_workingTemperature;
  }
  _statisticsTemperatureCount = c_statisticsTemperatureCount;
}

ThermometerClass GB_Thermometer;

