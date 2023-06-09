#ifndef _CONFIG_H_
#define _CONFIG_H_

#pragma once
#include <Arduino.h> 
#include <data_transfer.h>          // Brings dataSave and dataDriver structs into scope


// -----------------------------------------------------------------------------
// Flow sensor specific settings, adjust if needed:
// -----------------------------------------------------------------------------
const int FLOW_SENSOR_ADDRESS = 0x08;               // Sensor I2C Address for BOTH inlet and outlet sensors, using separate I2C busses for each sensor
const float SCALE_FACTOR_FLOW = 32.0;              // Scale Factor for flow rate measurement
const float SCALE_FACTOR_TEMP = 200.0;              // Scale Factor for temperature measurement
const char UNIT_FLOW[] = " ml/min";                 // physical unit of the flow rate measurement
const char UNIT_TEMP[] = " deg C";                  // physical unit of the temperature measurement
const int MEASUREMENT_MODE_B1 = 0x36;               // First byte of the measurement mode command, IPA calibration
const int MEASUREMENT_MODE_B2 = 0x08;               // Second byte of the measurement mode command, IPA calibration
float inlet_flow_sensor_ml_min;            // mL/min from the inlet flow sensor
float outlet_flow_sensor_ml_min;           // mL/min from the outlet flow sensor

// -----------------------------------------------------------------------------
// Rope heater control specific settings, adjust if needed:
// -----------------------------------------------------------------------------
float rope_Kp = 10.0;
float rope_Ki = 0.01;
float rope_Kd = 0.25;
float inlet_fluid_temp_setpoint = 50;              // Setpoint for the inlet fluid temperature, can change this on the screen
float inlet_fluid_temp_max = 60;
float inlet_fluid_temp_min = 20;
float inlet_fluid_temp_measured;                   // Measured inlet fluid temperature
float rope_control_output;                         // Control output from the PID controller
const uint16_t window_size_ms = 5000;              // Window size for the PID controller
const byte debounce_delay_ms = 15;                 // Debounce delay in ms for the PID controller
uint16_t window_start_time;                        // Start time for the PID controller, don't set
uint16_t next_switch_time;                         // Calculated switch time for the PID controller, don't set
bool relay_status = false;                         // PID controller window open flag
bool rope_heater_enable = false;                   // Enable/disable the rope heater

// -----------------------------------------------------------------------------
// Other random settings, adjust if needed:
// -----------------------------------------------------------------------------
// Status LED blink variables
bool blink_led_state  = false;             // flag for blinking the status LED on/off
const int blink_delay_ms = 1000;           // delay in ms

// Heat flux of heaters (0 to 100)
float heat_flux = 0.0;                   // Heat flux of the heaters in W/m^2
float max_heat_flux = 100.0;             // Maximum possible heat flux of the heaters in W/m^2
bool heater_block_enable = false;        // Enable/disable the heater block heater cartridge
uint16_t heat_flux_pwm;

// Measurement delay time (for flow sensors, valve position, valve diff pressure), and update delay time for digital output
const int measurement_update_delay_ms = 100;    // Delay between measurements and update output in ms

// Serial communication settings
bool print_to_serial = false;            // print to serial port
const int SERIAL_BAUD = 115200;          // baud rate for serial communication
bool wait_for_serial = false;            // wait for serial connection before starting program
uint16_t measure_and_send_data_delay_ms = 1000/25;        // delay between sending data over serial port in ms

// Analog (ADC) settings
int16_t analog_resolution = 12;                          // bits of resolution on ADC measurements
int16_t max_analog = pow(2, analog_resolution)-1;        // Max analog resolution 2^(analog_resolution)-1 = 4095 for 12 bit, 2047 for 11 bit, 1023 for 10 bit
const float analog_vref = 3.3;                                 // Analog reference voltage, 5.0V for ArduinoMega2560      

// Min and max values for the conversion functions
float min_pwm_output = 0.0;
float max_pwm_output = 255.0;
float min_flow_rate = 0.0;         // ml/min
float max_flow_rate = 1000.0;      // ml/min
float min_temp = 0.0;              // deg C
float max_temp = 100.0;            // deg C
float min_heat_flux = 0.0;         // W/m^2

// -----------------------------------------------------------------------------
// Piezo variables
// -----------------------------------------------------------------------------
uint8_t signal_type_piezo_1 = 0;    // 0 = sine, 1 = sawtooth, 2 = square, 3 = triangle, 4 = noise, 5 = sweep. Note the first 4 are defined as such in synth_waveform.h
uint8_t signal_type_piezo_2 = 0;    // 0 = sine, 1 = sawtooth, 2 = square, 3 = triangle, 4 = noise, 5 = sweep. Note the first 4 are defined as such in synth_waveform.h
float piezo_1_freq = 1000.0;        // Frequency of piezo 1 in Hz
float piezo_2_freq = 1000.0;        // Frequency of piezo 2 in Hz
float piezo_1_vpp  = 2.0;           // Voltage Peak-to-Peak of piezo 1 in V, this NEEDS to be divided by 100 for the amplifier
float piezo_2_vpp  = 2.0;           // Voltage Peak-to-Peak of piezo 2 in V, this NEEDS to be divided by 100 for the amplifier
float piezo_1_phase = 0.0;          // Phase of piezo 1 in degrees
float piezo_2_phase = 0.0;          // Phase of piezo 2 in degrees
bool piezo_1_enable = false;        // Enable/disable piezo 1
bool piezo_2_enable = false;        // Enable/disable piezo 2


// -----------------------------------------------------------------------------
// Thermistor variables
// -----------------------------------------------------------------------------
const uint16_t thermistor_R1 = 5100;               // ohms for the series resistor for the thermistors
float thermistor_1_temp = 0.0;                      // Temperature of thermistor 1 in deg C, upper R1
float thermistor_2_temp = 0.0;                      // Temperature of thermistor 2 in deg C, upper R1
float thermistor_3_temp = 0.0;                      // Temperature of thermistor 3 in deg C, upper R1
float thermistor_4_temp = 0.0;                      // Temperature of thermistor 4 in deg C, upper R1
float thermistor_5_temp = 0.0;                      // Temperature of thermistor 5 in deg C, upper R1
float thermistor_6_temp = 0.0;                      // Temperature of thermistor 6 in deg C, upper R1
float thermistor_7_temp = 0.0;                      // Temperature of thermistor 7 in deg C, lower R1
float thermistor_8_temp = 0.0;                      // Temperature of thermistor 8 in deg C, lower R1
float thermistor_9_temp = 0.0;                      // Temperature of thermistor 9 in deg C, lower R1
float thermistor_10_temp = 0.0;                     // Temperature of thermistor 10 in deg C, lower R1
float thermistor_11_temp = 0.0;                     // Temperature of thermistor 11 in deg C, lower R1
float thermistor_12_temp = 0.0;                     // Temperature of thermistor 12 in deg C, lower R1
float thermistor_13_temp = 0.0;                     // Temperature of thermistor 13 in deg C, lower R1  
float thermistor_14_temp = 0.0;                     // Temperature of thermistor 14 in deg C, lower R1


// -----------------------------------------------------------------------------
// Conversions functions for inputs and outputs
// -----------------------------------------------------------------------------

// Map function for float inputs
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


#include "RT_Table.h"  // The header file we created

// Function to get the resistance across the thermistor
float calculate_thermistor_resistance(int analogReadValue, int maxAnalogRead, float referenceVoltage, int seriesResistor) {
    float thermistorResistance = seriesResistor / (maxAnalogRead / float(analogReadValue) - 1.0);
    return thermistorResistance;
}


// Function to estimate temperature using the lookup table
float lookup_thermistor_temperature(float resistance) {
    for (int i = 0; i < RT_TABLE_SIZE - 1; i++) {
        float lowResistance = RT_TABLE[i + 1][0];
        float highResistance = RT_TABLE[i][0];

        // if the resistance is within a range in the table
        if (lowResistance <= resistance && resistance <= highResistance) {
            float lowTemp = RT_TABLE[i + 1][1];
            float highTemp = RT_TABLE[i][1];

            // linear interpolation
            float temp = lowTemp + (resistance - lowResistance) * (highTemp - lowTemp) / (highResistance - lowResistance);
            return temp;
        }
    }
    // if the resistance is not within the range of the table, return an error value
    return -999;
}

// Function to convert analog read value to temperature
float convert_thermistor_analog_to_celcius(int analogReadValue, int maxAnalogRead, float referenceVoltage, int seriesResistor) {
    float resistance = calculate_thermistor_resistance(analogReadValue, maxAnalogRead, referenceVoltage, seriesResistor);
    float temperature = lookup_thermistor_temperature(resistance);
    return temperature;
}

#endif