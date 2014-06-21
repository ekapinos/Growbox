#ifndef GB_Termometer_h
#define GB_Termometer_h

// Termometer
#include <DallasTemperature.h>

#include "Logger.h"

class GB_Termometer{

private:
  // Pass our oneWire reference to Dallas Temperature. 
  static DallasTemperature g_dallasTemperature;
  static DeviceAddress g_thermometerOneWireAddress;

  static float g_temperature;
  static double g_temperatureSumm;
  static int g_temperatureSummCount;

public:

  static void start(){
    g_dallasTemperature.begin();
    while(g_dallasTemperature.getDeviceCount() == 0){
      GB_Logger::logError(ERROR_TERMOMETER_DISCONNECTED);
      g_dallasTemperature.begin();
    }  
    GB_Logger::stopLogError(ERROR_TERMOMETER_DISCONNECTED);

    g_dallasTemperature.getAddress(g_thermometerOneWireAddress, 0); // search for devices on the bus and assign based on an index.
  }

  // TODO rename
  static boolean checkTemperature(){

    if(!g_dallasTemperature.requestTemperaturesByAddress(g_thermometerOneWireAddress)){
      GB_Logger::logError(ERROR_TERMOMETER_DISCONNECTED);
      return false;
    };

    float freshTemperature = g_dallasTemperature.getTempC(g_thermometerOneWireAddress);

    if ((int)freshTemperature == 0){
      GB_Logger::logError(ERROR_TERMOMETER_ZERO_VALUE);  
      return false;
    }

    g_temperatureSumm += freshTemperature;
    g_temperatureSummCount++;

    boolean forceLog = 
      GB_Logger::stopLogError(ERROR_TERMOMETER_ZERO_VALUE) |
      GB_Logger::stopLogError(ERROR_TERMOMETER_DISCONNECTED); 
    if (forceLog) {
      getTemperature(true);
    }
    else if (g_temperatureSummCount > 100){
      getTemperature(); // prevents overflow 
    }

    return true;
  }

  static float getTemperature(boolean forceLog = false){

    if (g_temperatureSummCount == 0){
      return g_temperature; 
    }

    float freshTemperature = g_temperatureSumm/g_temperatureSummCount;

    if (((int)freshTemperature != (int)g_temperature) || forceLog) {          
      GB_Logger::logTemperature((byte)freshTemperature);
    }

    g_temperature = freshTemperature;

    g_temperatureSumm = 0.0;
    g_temperatureSummCount = 0;

    return g_temperature;
  }

  /////////////////////////////////////////////////////////////////////
  //                        GROWBOX COMMANDS                         //
  /////////////////////////////////////////////////////////////////////


  static void getStatistics(float* workingTemperature, float* statisticsTemperature, int* statisticsCount){
    (*workingTemperature) = g_temperature;

    if (g_temperatureSummCount != 0){
      (*statisticsTemperature) = g_temperatureSumm/g_temperatureSummCount;
    } 
    else {
      (*statisticsTemperature) = g_temperature;
    }
    *statisticsCount = g_temperatureSummCount;
  }



};

#endif






