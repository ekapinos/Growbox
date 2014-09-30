#include "Thermometer.h"
#include "Logger.h"
#include "StorageHelper.h"

// public:

ThermometerClass::ThermometerClass(OneWire* oneWirePin) :
    c_dallasTemperature(oneWirePin), c_lastTemperature(NAN),
    c_statisticsTemperatureSumm(0.0), c_statisticsTemperatureCount(0) {
}

float ThermometerClass::getHardwareTemperature(boolean logOnError) {

  c_dallasTemperature.begin();

  if (c_dallasTemperature.getDeviceCount() == 0) {
    if (logOnError) {
      GB_Logger.logError(ERROR_TERMOMETER_DISCONNECTED);
    }
    return NAN;
  }

  DeviceAddress oneWireAddress;

  if (!c_dallasTemperature.getAddress(oneWireAddress, 0)) {
    if (logOnError) {
      GB_Logger.logError(ERROR_TERMOMETER_DISCONNECTED);
    }
    return NAN;
  }

  if (!c_dallasTemperature.requestTemperaturesByAddress(oneWireAddress)) {
    if (logOnError) {
      GB_Logger.logError(ERROR_TERMOMETER_DISCONNECTED);
    }
    return NAN;
  };

  float temperature = c_dallasTemperature.getTempC(oneWireAddress);

  if ((int)temperature == 0) {
    if (logOnError) {
      GB_Logger.logError(ERROR_TERMOMETER_ZERO_VALUE);
    }
    return NAN;
  }

  return temperature;
}

boolean ThermometerClass::isPresent() {
  return !isnan(getHardwareTemperature(false));
}


void ThermometerClass::setUseThermometer(boolean flag){
  if (flag == isUseThermometer()){
    return;
  }
  GB_StorageHelper.setUseThermometer(flag);
  c_lastTemperature = NAN;
  c_statisticsTemperatureSumm = 0.0;
  c_statisticsTemperatureCount = 0;
  if (flag) {
    updateStatistics();
  }
}
boolean ThermometerClass::isUseThermometer(){
  return GB_StorageHelper.isUseThermometer();
}

void ThermometerClass::updateStatistics() {
  if (!isUseThermometer()) {
    return;
  }
  float freshTemperature = getHardwareTemperature(true);
  if (isnan(freshTemperature)) {
    return; // Errors already logged
  }

  c_statisticsTemperatureSumm += freshTemperature;
  c_statisticsTemperatureCount++;

  boolean forceLog = GB_Logger.stopLogError(ERROR_TERMOMETER_ZERO_VALUE) | GB_Logger.stopLogError(ERROR_TERMOMETER_DISCONNECTED);

  if (forceLog || c_statisticsTemperatureCount > 200) { // prevents overflow (3 times per minute max fan period 60 minutes)
    getTemperatureAndClearStatistics();
  }
}

float ThermometerClass::getTemperatureAndClearStatistics() {
  if (!isUseThermometer()) {
    return NAN; // Used as normal value
  }
  float freshTemperature;
  if (c_statisticsTemperatureCount == 0) {
    // Lets try to get temperature directly
    freshTemperature = getHardwareTemperature(true);
  }
  else {
    freshTemperature = c_statisticsTemperatureSumm / c_statisticsTemperatureCount;
  }
  if (!isnan(freshTemperature)) {
    if (isnan(c_lastTemperature) || ((int)freshTemperature != (int)c_lastTemperature)) {
      GB_Logger.logTemperature((byte)freshTemperature);
    }
  }
  c_lastTemperature = freshTemperature;
  c_statisticsTemperatureSumm = 0.0;
  c_statisticsTemperatureCount = 0;

  return freshTemperature;
}

float ThermometerClass::getLastTemperature() {
  return c_lastTemperature;
}

float ThermometerClass::getForecastTemperature() {
  if (c_statisticsTemperatureCount != 0) {
    return c_statisticsTemperatureSumm / c_statisticsTemperatureCount;
  }
  else {
    return c_lastTemperature;
  }
}

int ThermometerClass::getForecastMeasurementCount() {
  return c_statisticsTemperatureCount;
}

// Pass our oneWire reference to Dallas Temperature.
ThermometerClass GB_Thermometer(&g_oneWirePin);

