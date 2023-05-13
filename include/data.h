#ifndef _DATA_H_
#define _DATA_H_

#pragma once
#include <Arduino.h>          // Arduino library for Arduino specific types


// Holds the data struct which is passed between the Teensy 4.1 (Monitor) and Mega

struct dataMEGA {
    float heat_flux;
    float inlet_flow_sensor_ml_min;
    float outlet_flow_sensor_ml_min;
    float inlet_valve_ml_min;
    float inlet_fluid_temp;
} data_mega;

struct dataDriver {
    float 
}


#endif // _DATA_H_