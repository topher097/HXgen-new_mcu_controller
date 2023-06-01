#ifndef _DATA_H_
#define _DATA_H_

#pragma once
#include <Arduino.h>          // Arduino library for Arduino specific types


// Thermistor data
#define NUM_THERMISTORS 14    // Number of thermistors in the system
float thermistor_temps[NUM_THERMISTORS]; // Array to hold the thermistor temperatures



// ------------------ MONITOR AND PC DATA STRUCTS ------------------ //
struct __attribute__((packed)) dataSave {
    uint32_t time_ms;                       // ms since start of recording
    uint32_t time_us;                       // us since start of recording
    float inlet_fluid_temp;                 // Temperature of the inlet fluid in deg milli C (from thermocouple)
    float inlet_fluid_temp_setpoint;        // Setpoint for the inlet fluid temperature, can change this on the screen
    bool heater_block_enable;               // Enable/disable the heater block heater cartridges
    bool rope_heater_enable;                // Enable/disable the rope heater
    float heat_flux;                        // Heat flux of the heaters in W/m^2
    float thermistor_1_temp_c;              // Temperature of the thermistor 1 in deg C
    float thermistor_2_temp_c;              // Temperature of the thermistor 2 in deg C
    float thermistor_3_temp_c;              // Temperature of the thermistor 3 in deg C
    float thermistor_4_temp_c;              // Temperature of the thermistor 4 in deg C
    float thermistor_5_temp_c;              // Temperature of the thermistor 5 in deg C
    float thermistor_6_temp_c;              // Temperature of the thermistor 6 in deg C
    float thermistor_7_temp_c;              // Temperature of the thermistor 7 in deg C
    float thermistor_8_temp_c;              // Temperature of the thermistor 8 in deg C
    float thermistor_9_temp_c;              // Temperature of the thermistor 9 in deg C
    float thermistor_10_temp_c;             // Temperature of the thermistor 10 in deg C
    float thermistor_11_temp_c;             // Temperature of the thermistor 11 in deg C
    float thermistor_12_temp_c;             // Temperature of the thermistor 12 in deg C
    float thermistor_13_temp_c;             // Temperature of the thermistor 13 in deg C
    float thermistor_14_temp_c;             // Temperature of the thermistor 14 in deg C
} monitor_to_pc_data;

struct __attribute__((packed)) dataPCtoMonitor {
    bool reset_time;                        // Reset the time to 0
    bool heater_block_enable;               // Enable/disable the heater block heater cartridges
    bool rope_heater_enable;                // Enable/disable the rope heater
    float heat_flux;                        // Heat flux of the heaters in W/m^2
    float inlet_fluid_temp_setpoint;        // Setpoint for the inlet fluid temperature, can change this on the screen
} pc_to_monitor_data;


// ------------------------- DRIVER AND PC DATA STRUCTS ------------------------- //
struct __attribute((packed)) dataDriverToPC {
    uint32_t time_ms;               // Time in milliseconds since the program started
    uint32_t time_us;               // Time in microseconds since the program started
    uint8_t signal_type_piezo_1;    // 0 = sine, 1 = square, 2 = triangle, 3 = sawtooth, 4 = noise
    uint8_t signal_type_piezo_2;    // 0 = sine, 1 = square, 2 = triangle, 3 = sawtooth, 4 = noise
    bool piezo_1_enable;            // Enable pin for piezo driver 1
    bool piezo_2_enable;            // Enable pin for piezo driver 2
    float piezo_1_freq;             // Frequency of left channel piezo in Hz
    float piezo_2_freq;             // Frequency of right channel piezo in Hz
    float piezo_1_vpp;              // Amplitude of left channel sine wave in voltage p-p need to make sure this is divided by 100 for signal
    float piezo_2_vpp;              // Amplitude of right channel sine wave in voltage p-p need to make sure this is divided by 100 for signal
    float piezo_1_phase;            // Phase of left channel signal in degrees
    float piezo_2_phase;            // Phase of right channel signal in degrees
    float inlet_flow_sensor_ml_min;         // mL/min from the inlet flow sensor
    float outlet_flow_sensor_ml_min;        // mL/min from the outlet flow sensor
} driver_to_pc_data;

struct __attribute__((packed)) dataPCtoDriver {
    bool reset_time;            // Reset the time to 0
    uint8_t signal_type_piezo_1;    // 0 = sine, 1 = square, 2 = triangle, 3 = sawtooth, 4 = noise
    uint8_t signal_type_piezo_2;    // 0 = sine, 1 = square, 2 = triangle, 3 = sawtooth, 4 = noise
    bool piezo_1_enable;        // Enable pin for piezo driver 1
    bool piezo_2_enable;        // Enable pin for piezo driver 2
    float piezo_1_freq;         // Frequency of left channel piezo in Hz
    float piezo_2_freq;         // Frequency of right channel piezo in Hz
    float piezo_1_vpp;          // Amplitude of left channel sine wave in voltage p-p need to make sure this is divided by 100 for signal
    float piezo_2_vpp;          // Amplitude of right channel sine wave in voltage p-p need to make sure this is divided by 100 for signal
    float piezo_1_phase;        // Phase of left channel signal in degrees
    float piezo_2_phase;        // Phase of right channel signal in degrees
} pc_to_driver_data;
#endif // _DATA_H_