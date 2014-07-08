#include "Thermometer.h"

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature GB_Thermometer::dallasTemperature(&g_oneWirePin);
DeviceAddress GB_Thermometer::oneWireAddress;

// Visible only in this file
float GB_Thermometer::workingTemperature = 0.0;
double GB_Thermometer::statisticsTemperatureSumm = 0.0;
int GB_Thermometer::statisticsTemperatureCount = 0;

