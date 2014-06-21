#include "Thermometer.h"

// Visible only in this file
float GB_Thermometer::g_temperature = 0.0;
double GB_Thermometer::g_temperatureSumm = 0.0;
int GB_Thermometer::g_temperatureSummCount = 0;

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature GB_Thermometer::g_dallasTemperature(&g_oneWirePin);
DeviceAddress GB_Thermometer::g_oneWireAddress;


