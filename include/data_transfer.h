#ifndef _DATA_H_
#define _DATA_H_

#pragma once
#include <Arduino.h>          // Arduino library for Arduino specific types

// Holds the data struct which is saved to the SD card
struct dataSave {
    uint32_t time_stamp_ms;                 // ms since start of recording
    uint32_t time_stamp_us;                 // us since start of recording
    float heat_flux;                        // Heat flux of the heaters in W/m^2
    float inlet_flow_sensor_ml_min;         // mL/min from the inlet flow sensor
    float outlet_flow_sensor_ml_min;        // mL/min from the outlet flow sensor
    float inlet_fluid_temp;                 // Temperature of the inlet fluid in deg milli C (from thermocouple)
    float inlet_fluid_temp_setpoint;        // Setpoint for the inlet fluid temperature, can change this on the screen
    bool rope_heater_enable;                // Enable/disable the rope heater
    bool heater_block_enable;               // Enable/disable the heater block heater cartridges
    float piezo_1_freq;                     // Frequency of left channel piezo in Hz
    float piezo_2_freq;                     // Frequency of right channel piezo in Hz
    float piezo_1_vpp;                      // Amplitude of left channel sine wave in voltage p-p (voltage to piezo is 100x this from amplifier)
    float piezo_2_vpp;                      // Amplitude of right channel sine wave in voltage p-p (voltage to piezo is 100x this from amplifier)
    float piezo_1_phase;                    // Phase of left channel signal in radians
    float piezo_2_phase;                    // Phase of right channel signal in radians
    bool piezo_1_enable;                    // Bool for enabling/disabling the left channel piezo
    bool piezo_2_enable;                    // Bool for enabling/disabling the right channel piezo
    uint32_t numrecords;                    // Number of records saved to the SD card (one record is one line in csv file)      
    uint8_t *cptr;                          // pointer to last storage location to check out-of-bounds writes
};

// Holds the data struct which is sent to the Driver Teensy
struct dataMonitorToDriver {
    uint32_t piezo_1_freq;      // Frequency of left channel piezo in Hz
    uint32_t piezo_2_freq;      // Frequency of right channel piezo in Hz
    uint16_t piezo_1_vpp;       // Amplitude of sine wave 1 (left cahnnel);
    uint16_t piezo_2_vpp;       // Amplitude of sine wave 2 (right channel);
    uint16_t piezo_1_phase;     // Phase of left channel signal in degrees
    uint16_t piezo_2_phase;     // Phase of right channel signal in degrees
    bool piezo_1_enable;        // Enable pin for piezo driver 1
    bool piezo_2_enable;        // Enable pin for piezo driver 2
    
} monitor_to_driver_data;

// Holds the data struct which is sent to the Monitor Teensy
struct dataDriverToMonitor {
    float inlet_flow_sensor_ml_min;         // mL/min from the inlet flow sensor
    float outlet_flow_sensor_ml_min;        // mL/min from the outlet flow sensor
} driver_to_monitor_data;

struct dataMega {
    float inlet_flow_sensor_ml_min;         // mL/min from the inlet flow sensor
    float outlet_flow_sensor_ml_min;        // mL/min from the outlet flow sensor
} mega_data;

#endif // _DATA_H_