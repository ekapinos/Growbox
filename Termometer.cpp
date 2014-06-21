#include "Termometer.h"

// Visible only in this file
float GB_Termometer::g_temperature = 0.0;
double GB_Termometer::g_temperatureSumm = 0.0;
int GB_Termometer::g_temperatureSummCount = 0;

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature GB_Termometer::g_dallasTemperature(&g_oneWirePin);
DeviceAddress GB_Termometer::g_thermometerOneWireAddress;


