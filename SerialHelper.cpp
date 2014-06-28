#include "SerialHelper.h"

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

/*volatile*/boolean GB_SerialHelper::useSerialMonitor = false;
/*volatile*/boolean GB_SerialHelper::useSerialWifi = false;
boolean GB_SerialHelper::serialWifiError = false;

String GB_SerialHelper::wifiSID;
String GB_SerialHelper::wifiPass;


