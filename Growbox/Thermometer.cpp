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
    showMessage(F("c_dallasTemperature.getDeviceCount() == 0"));
    if (logOnError) {
      GB_Logger.logError(ERROR_TERMOMETER_DISCONNECTED);
    }
    return NAN;
  }

  DeviceAddress oneWireAddress;

  if (!c_dallasTemperature.getAddress(oneWireAddress, 0)) {
    showMessage(F("!c_dallasTemperature.getAddress(oneWireAddress, 0)"));
    if (logOnError) {
      GB_Logger.logError(ERROR_TERMOMETER_DISCONNECTED);
    }
    return NAN;
  }

  if (!c_dallasTemperature.requestTemperaturesByAddress(oneWireAddress)) {
    showMessage(F("!c_dallasTemperature.requestTemperaturesByAddress(oneWireAddress)"));
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
  return !isnan(getHardwareTemperature());
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

  boolean wasZeroError         = GB_Logger.stopLogError(ERROR_TERMOMETER_ZERO_VALUE);
  boolean wasDisconnectedError = GB_Logger.stopLogError(ERROR_TERMOMETER_DISCONNECTED);

  if (wasZeroError || wasDisconnectedError) {
    GB_Logger.logEvent(EVENT_THERMOMETER_RESTORED);
  }

  if (c_statisticsTemperatureCount > 200) { // prevents overflow (3 times per minute max fan period 60 minutes)
    c_statisticsTemperatureSumm = c_statisticsTemperatureSumm / c_statisticsTemperatureCount;
    c_statisticsTemperatureCount = 1;
  }
}

float ThermometerClass::getTemperatureAndClearStatistics() {
  if (!isUseThermometer()) {
    return NAN; // Used as normal value
  }
  float freshTemperature;
  if (c_statisticsTemperatureCount == 0) {
    updateStatistics();
  }
  if (c_statisticsTemperatureCount == 0) {
    freshTemperature = NAN; // All updateStatistics() failed
  } else {
    freshTemperature = c_statisticsTemperatureSumm / c_statisticsTemperatureCount;
  }
  if (!isnan(freshTemperature)) {
    if (isnan(c_lastTemperature) || ((byte)freshTemperature != (byte)c_lastTemperature)) {
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
  if (c_statisticsTemperatureCount == 0) {
    updateStatistics();
  }
  if (c_statisticsTemperatureCount == 0) {
    return NAN; // All updateStatistics() failed
  } else {
    return c_statisticsTemperatureSumm / c_statisticsTemperatureCount;
  }
}

int ThermometerClass::getForecastMeasurementCount() {
  return c_statisticsTemperatureCount;
}

// Pass our oneWire reference to Dallas Temperature.
ThermometerClass GB_Thermometer(&g_oneWirePin);

