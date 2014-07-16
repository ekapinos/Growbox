#ifndef Global_h
#define Global_h

#include <OneWire.h>

#include "ArduinoPatch.h"
#include "StringUtils.h"

/////////////////////////////////////////////////////////////////////
//                     HARDWARE CONFIGURATION                      //
/////////////////////////////////////////////////////////////////////

// relay pins
const byte LIGHT_PIN = 3;
const byte FAN_PIN = 4;
const byte FAN_SPEED_PIN = 5;

// 1-Wire pins
const byte ONE_WIRE_PIN = 8;

// status pins
const byte USE_SERIAL_MONOTOR_PIN = 11;
const byte ERROR_PIN = 12;
const byte BREEZE_PIN = LED_BUILTIN; //13

/////////////////////////////////////////////////////////////////////
//                     SOFTWARE CONFIGURATION                      //
/////////////////////////////////////////////////////////////////////

// Light scheduler
const byte UP_HOUR = 1;
const byte DOWN_HOUR = 9;  // 16/8

const float TEMPERATURE_DAY = 26.0;  //23-25, someone 27 
const float TEMPERATURE_NIGHT = 22.0;  // 18-22, someone 22
const float TEMPERATURE_CRITICAL = 35.0; // 35 max, 40 die
const float TEMPERATURE_DELTA = 3.0;   // +/-(3-4) Ok

/////////////////////////////////////////////////////////////////////
//                          PIN CONSTANTCS                         //
/////////////////////////////////////////////////////////////////////

// Relay consts
const byte RELAY_ON = LOW, RELAY_OFF = HIGH;

// Serial consts
const byte SERIAL_MONITOR_ON = HIGH, SERIAL_MONITOR_OFF = LOW;

// Fun speed
const byte FAN_SPEED_MIN = RELAY_OFF;
const byte FAN_SPEED_MAX = RELAY_ON;

/////////////////////////////////////////////////////////////////////
//                             DELAY'S                             //
/////////////////////////////////////////////////////////////////////

// Minimum Growbox reaction time
const int UPDATE_THEMPERATURE_STATISTICS_DELAY = 20; //20 sec 
const int UPDATE_WIFI_STATUS_DELAY = 20; //20 sec 
const int UPDATE_SERIAL_MONITOR_STATUS_DELAY = 1;
const int UPDATE_GROWBOX_STATE_DELAY = 5*60; // 5 min 
const int UPDATE_BREEZE_DELAY = 1;

// error blinks in milleseconds and blink sequences
const word ERROR_SHORT_SIGNAL_MS = 100;  // -> 0
const word ERROR_LONG_SIGNAL_MS = 400;   // -> 1
const word ERROR_DELAY_BETWEEN_SIGNALS_MS = 150;


/////////////////////////////////////////////////////////////////////
//                        GLOBAL VARIABLES                         //
/////////////////////////////////////////////////////////////////////

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
extern OneWire g_oneWirePin;
extern boolean g_isGrowboxStarted;
extern byte g_isDayInGrowbox;
extern boolean g_useSerialMonitor;


/////////////////////////////////////////////////////////////////////
//                         GLOBAL STRINGS                          //
/////////////////////////////////////////////////////////////////////

const char S_WIFI_DEFAULT_SSID[] PROGMEM  = "Growbox"; //WPA2 in AP mode
const char S_WIFI_DEFAULT_PASS[] PROGMEM  = "ingodwetrust";

const char S_empty[] PROGMEM  = "";
const char S___ [] PROGMEM  = "  ";
const char S_CRLF[] PROGMEM  = "\r\n";
const char S_CRLFCRLF[] PROGMEM  = "\r\n\r\n";
const char S_Connected[] PROGMEM  = "Connected";
const char S_Disconnected[] PROGMEM  = "Disconnected";
const char S_Enabled[] PROGMEM  = "Enabled";
const char S_Disabled[] PROGMEM  = "Disabled";
const char S_Temperature[] PROGMEM  = "Temperature";
const char S_Free_memory[] PROGMEM  = "Free memory: ";
const char S_bytes[] PROGMEM  = " bytes";
const char S_PlusMinus [] PROGMEM  = "+/-";
const char S_Next [] PROGMEM  = " > ";

/////////////////////////////////////////////////////////////////////
//                            COLLECTIONS                          //
/////////////////////////////////////////////////////////////////////

template <class T> 
class Node {
public: 
  T* data;
  Node* nextNode;  
};

template <class T> 
class Iterator {
  Node<T>* currentNode; 

public:
  Iterator(Node<T>* currentNode) : 
  currentNode(currentNode){
  }
  boolean hasNext(){
    return (currentNode != 0);
  }
  T* getNext(){
    T* rez = currentNode->data; 
    currentNode = currentNode->nextNode;
    return rez;
  }
};

template <class T> 
class LinkedList {
private: 
  Node<T>* lastNode;

public:  
  LinkedList():
  lastNode(0){
  }

  ~LinkedList(){

    while(lastNode != 0){
      Node<T>* currNode  = lastNode;
      lastNode = lastNode->nextNode;
      delete currNode;
    }
  }

  void add(T* data){
    Node<T>* newNode = new Node<T>;
    newNode->data = data;
    newNode->nextNode = lastNode;
    lastNode = newNode;
  }
  
  Iterator<T> getIterator(){
    return Iterator<T>(lastNode);
  }
};

#endif






