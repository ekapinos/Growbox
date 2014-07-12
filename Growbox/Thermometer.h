#ifndef GB_Thermometer_h
#define GB_Thermometer_h

// Termometer
#include <DallasTemperature.h>

#include "Logger.h"

class GB_Thermometer{

private:
  // Pass our oneWire reference to Dallas Temperature. 
  static DallasTemperature dallasTemperature;
  static DeviceAddress oneWireAddress;

  static float workingTemperature;
  static double statisticsTemperatureSumm;
  static int statisticsTemperatureCount;

public:

  static void start(){
    dallasTemperature.begin();
    while(dallasTemperature.getDeviceCount() == 0){
      GB_LOGGER.logError(ERROR_TERMOMETER_DISCONNECTED);
      dallasTemperature.begin();
    }  
    GB_LOGGER.stopLogError(ERROR_TERMOMETER_DISCONNECTED);

    dallasTemperature.getAddress(oneWireAddress, 0); // search for devices on the bus and assign based on an index.
  }

  // TODO rename
  static boolean updateStatistics(){

    if(!dallasTemperature.requestTemperaturesByAddress(oneWireAddress)){
      GB_LOGGER.logError(ERROR_TERMOMETER_DISCONNECTED);
      return false;
    };

    float freshTemperature = dallasTemperature.getTempC(oneWireAddress);

    if ((int)freshTemperature == 0){
      GB_LOGGER.logError(ERROR_TERMOMETER_ZERO_VALUE);  
      return false;
    }

    statisticsTemperatureSumm += freshTemperature;
    statisticsTemperatureCount++;

    boolean forceLog = 
      GB_LOGGER.stopLogError(ERROR_TERMOMETER_ZERO_VALUE) |
      GB_LOGGER.stopLogError(ERROR_TERMOMETER_DISCONNECTED); 
    if (forceLog) {
      getTemperature(true);
    }
    else if (statisticsTemperatureCount > 100){
      getTemperature(); // prevents overflow 
    }

    return true;
  }

  static float getTemperature(boolean forceLog = false){

    if (statisticsTemperatureCount == 0){
      return workingTemperature; 
    }

    float freshTemperature = statisticsTemperatureSumm/statisticsTemperatureCount;

    if (((int)freshTemperature != (int)workingTemperature) || forceLog) {          
      GB_LOGGER.logTemperature((byte)freshTemperature);
    }

    workingTemperature = freshTemperature;

    statisticsTemperatureSumm = 0.0;
    statisticsTemperatureCount = 0;

    return workingTemperature;
  }

  /////////////////////////////////////////////////////////////////////
  //                        GROWBOX COMMANDS                         //
  /////////////////////////////////////////////////////////////////////

  static void getStatistics(float &_workingTemperature, float &_statisticsTemperature, int &_statisticsTemperatureCount){
    _workingTemperature = workingTemperature;

    if (statisticsTemperatureCount != 0){
      _statisticsTemperature = statisticsTemperatureSumm/statisticsTemperatureCount;
    } 
    else {
      _statisticsTemperature = workingTemperature;
    }
    _statisticsTemperatureCount = statisticsTemperatureCount;
  }


};

#endif






