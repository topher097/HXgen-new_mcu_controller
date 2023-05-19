#ifndef _DATA_H_
#define _DATA_H_

#pragma once
#include <Arduino.h>          // Arduino library for Arduino specific types


// Holds the data struct which is saved to the SD card
struct dataSave {
    float heat_flux;
    float inlet_flow_sensor_ml_min;
    float outlet_flow_sensor_ml_min;
    float inlet_valve_ml_min;
    float inlet_fluid_temp;
    bool rope_heater_enable;
    bool heater_block_enable;
    float piezo_1_freq;
    float piezo_2_freq;
    float piezo_1_vpp;
    float piezo_2_vpp;
    float piezo_1_phase;
    float piezo_2_phase;
    float piezo_1_enable;
    float piezo_2_enable;
    bool start_stop_recording;
} save_data;


// Holds the data struct which is sent to the Driver Teensy
struct dataDriver {
    float piezo_1_freq;      // Frequency of left channel piezo in Hz
    float piezo_2_freq;      // Frequency of right channel piezo in Hz
    float piezo_1_vpp;       // Amplitude of sine wave 1 (left cahnnel);
    float piezo_2_vpp;       // Amplitude of sine wave 2 (right channel);
    float piezo_1_phase;     // Phase of left channel signal in degrees
    float piezo_2_phase;     // Phase of right channel signal in degrees
    int piezo_1_enable;      // Enable pin for piezo driver 1
    int piezo_2_enable;      // Enable pin for piezo driver 2
} data_driver;

#endif // _DATA_H_