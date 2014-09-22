#ifndef Global_h
#define Global_h

#include <OneWire.h>

#include "ArduinoPatch.h"
#include "StringUtils.h"

/////////////////////////////////////////////////////////////////////
//                     HARDWARE CONFIGURATION                      //
/////////////////////////////////////////////////////////////////////

// relay pins
const byte LIGHT_PIN = 2;
const byte FAN_PIN = 3;
const byte FAN_SPEED_PIN = 4;
const byte HEATER_PIN = 5;

// 1-Wire pins
const byte ONE_WIRE_PIN = 12;

// status pins
const byte ERROR_PIN = 11;
const byte BREEZE_PIN = LED_BUILTIN; //13

// Watering
const byte MAX_WATERING_SYSTEMS_COUNT = 4;
const byte WATERING_WET_SENSOR_IN_PINS[] = {A0, A1, A2, A3};
const byte WATERING_WET_SENSOR_POWER_PINS[] = {22, 24, 26, 28};
const byte WATERING_PUMP_PINS[] = {23, 25, 27, 29};

// hardware buttons
const byte HARDWARE_BUTTON_USE_SERIAL_MONOTOR_PIN = 53; // pullup used, 0 - enabled, 1 - disabled
const byte HARDWARE_BUTTON_RESET_FIRMWARE_PIN = 52; // pullup used, 0 - enabled, 1 - disabled (works when Serial Monitor enabled)

/////////////////////////////////////////////////////////////////////
//                          PIN CONSTANTCS                         //
/////////////////////////////////////////////////////////////////////

// Relay consts
const byte RELAY_ON = LOW, RELAY_OFF = HIGH;

// Serial consts
const byte HARDWARE_BUTTON_OFF = HIGH, HARDWARE_BUTTON_ON = LOW;

// Fun speed
const byte FAN_SPEED_MIN = RELAY_OFF;
const byte FAN_SPEED_MAX = RELAY_ON;

/////////////////////////////////////////////////////////////////////
//                             DELAY'S                             //
/////////////////////////////////////////////////////////////////////

// Main cycle
const time_t UPDATE_BREEZE_DELAY_SEC = 1;
const time_t UPDATE_GROWBOX_STATE_DELAY_SEC = 5 * 60; // 5 min
const time_t UPDATE_CONTROLLER_STATE_DELAY_SEC = 1;
const time_t UPDATE_CONTROLLER_CORE_HARDWARE_STATE_DELAY_SEC = 60; // 60 sec
const time_t UPDATE_CONTROLLER_AUTO_ADJUST_CLOCK_TIME_DELAY_SEC = SECS_PER_DAY; // 1 day
const time_t UPDATE_TERMOMETER_STATISTICS_DELAY_SEC = 20; //20 sec 
const time_t UPDATE_WEB_SERVER_STATUS_DELAY_SEC = 2 * 60; // 2 min

// Error blinks in milliseconds and blink sequences
const word ERROR_SHORT_SIGNAL_MS = 100;  // -> 0
const word ERROR_LONG_SIGNAL_MS = 400;   // -> 1
const word ERROR_DELAY_BETWEEN_SIGNALS_MS = 150;

// Web server
const int WEB_SERVER_AVERAGE_PAGE_LOAD_TIME_SEC = 3; // 3 sec

// Watering
const int WATERING_SYSTEM_TURN_ON_DELAY_SEC = 3; // 3 sec
const long WATERING_MAX_SCHEDULE_CORRECTION_TIME_SEC = 6 * 60 * 60; // hours
const long WATERING_ERROR_DELTA_SEC = 5 * 60; // 6 minutes

/////////////////////////////////////////////////////////////////////
//                          OTHER CONSTS                           //
/////////////////////////////////////////////////////////////////////

// Main cycle

const byte ON_COLD_FAN_TURN_OFF_INTERVAL_MIN = 15;  // On cold  0..15, 20..35, 40..55 - fan turned off
const byte ON_COLD_FAN_TURN_ON_INTERVAL_MIN = 5;    //         15..20, 35..40, 55..0  - fan turned on,
                                                    // better to use intervals aliquot to UPDATE_GROWBOX_STATE_DELAY_SEC
                                                    // and sum aliquot to 60 minutes

// Wi-Fi
const byte WI_FI_RECONNECT_ATTEMPTS_BEFORE_DEFAULT_PARAMS = 3;

/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
extern OneWire g_oneWirePin;
extern boolean g_useSerialMonitor;

/////////////////////////////////////////////////////////////////////
//                         GLOBAL STRINGS                          //
/////////////////////////////////////////////////////////////////////

const char S_WIFI_DEFAULT_SSID[] PROGMEM = "growbox"; //WPA2 in AP mode
const char S_WIFI_DEFAULT_PASS[] PROGMEM = "ingodwetrust";

const char S_CRLF[] PROGMEM = "\r\n";
const char S_CRLFCRLF[] PROGMEM = "\r\n\r\n";
const char S_Connected[] PROGMEM = "Connected";
const char S_Disconnected[] PROGMEM = "Disconnected";
const char S_Next[] PROGMEM = " > ";

/////////////////////////////////////////////////////////////////////
//                            COLLECTIONS                          //
/////////////////////////////////////////////////////////////////////

template<class T>
  class Node{
  public:
    T* data;
    Node* nextNode;
  };

template<class T>
  class Iterator{
    Node<T>* currentNode;

  public:
    Iterator(Node<T>* currentNode) :
        currentNode(currentNode) {
    }
    boolean hasNext() {
      return (currentNode != 0);
    }
    T* getNext() {
      T* rez = currentNode->data;
      currentNode = currentNode->nextNode;
      return rez;
    }
  };

template<class T>
  class LinkedList{
  private:
    Node<T>* lastNode;

  public:
    LinkedList() :
        lastNode(0) {
    }

    ~LinkedList() {

      while (lastNode != 0) {
        Node<T>* currNode = lastNode;
        lastNode = lastNode->nextNode;
        delete currNode;
      }
    }

    void add(T* data) {
      Node<T>* newNode = new Node<T>;
      newNode->data = data;
      newNode->nextNode = lastNode;
      lastNode = newNode;
    }

    Iterator<T> getIterator() {
      return Iterator<T>(lastNode);
    }
  };

#endif

