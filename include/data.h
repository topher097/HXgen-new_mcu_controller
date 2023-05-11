#ifndef _DATA_H_
#define _DATA_H_

#pragma once
#include <Arduino.h>          // Arduino library for Arduino specific types


// Holds the data struct which is passed from the main loop to the Screen class for drawing the screen

struct data {
    float heat_flux;
    float inlet_flow_sensor_ml_min;
    float outlet_flow_sensor_ml_min;
    float inlet_valve_ml_min;
    float inlet_fluid_temp;
} hx_data;

#endif // _DATA_H_