#include "SerialHelper.h"

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

/*volatile*/boolean GB_SerialHelper::useSerialMonitor = false;
/*volatile*/boolean GB_SerialHelper::useSerialWifi = false;
boolean GB_SerialHelper::s_restartWifi = false;
boolean GB_SerialHelper::s_restartWifiIfNoResponseAutomatically = true;



String GB_SerialHelper::s_wifiSID;
String GB_SerialHelper::s_wifiPass;

boolean GB_SerialHelper::s_wifiIsHeaderSended;
int GB_SerialHelper::s_wifiResponseAutoFlushConut;

