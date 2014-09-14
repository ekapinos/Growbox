#include "Global.h"

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire g_oneWirePin(ONE_WIRE_PIN);

byte g_isDayInGrowbox = -1;
boolean g_useSerialMonitor = false;

