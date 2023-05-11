#ifndef CONFIG_H
#define CONFIG_H

#pragma once
#include <Arduino.h>          // Arduino library for Arduino specific types
#include <data.h>

#ifdef __AVR_ATmega2560__
    #include <pins_mega.h>
#endif 
#ifdef __IMXRT1062__
    #include <pins_teensy.h>
#endif


// -----------------------------------------------------------------------------
// Flow sensor specific settings, adjust if needed:
// -----------------------------------------------------------------------------
const int FLOW_SENSOR_ADDRESS = 0x08;               // Sensor I2C Address for BOTH inlet and outlet sensors, using separate I2C busses for each sensor
const float SCALE_FACTOR_FLOW = 500.0;              // Scale Factor for flow rate measurement
const float SCALE_FACTOR_TEMP = 200.0;              // Scale Factor for temperature measurement
const char UNIT_FLOW[] = " ml/min";                 // physical unit of the flow rate measurement
const char UNIT_TEMP[] = " deg C";                  // physical unit of the temperature measurement
const int MEASUREMENT_MODE_B1 = 0x36;               // First byte of the measurement mode command, IPA calibration
const int MEASUREMENT_MODE_B2 = 0x08;               // Second byte of the measurement mode command, IPA calibration

// -----------------------------------------------------------------------------
// Rope heater control specific settings, adjust if needed:
// -----------------------------------------------------------------------------
float rope_Kp = 10.0;
float rope_Ki = 0.0;
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
boolean relay_status = false;                      // PID controller window open flag

// -----------------------------------------------------------------------------
// Other random settings, adjust if needed:
// -----------------------------------------------------------------------------
// Status LED blink variables
const int blink_delay_ms = 1000;           // delay in ms

// Heat flux of heaters (0 to 100)
float heat_flux = 0.0;                   // Heat flux of the heaters in W/m^2
float max_heat_flux = 100.0;             // Maximum possible heat flux of the heaters in W/m^2
uint16_t heat_flux_pwm;

// Measurement delay time (for flow sensors, valve position, valve diff pressure), and update delay time for digital output
const int measurement_update_delay_ms = 100;    // Delay between measurements and update output in ms

// Screen settings
const int screen_touch_resistance = 268;  // ohms b/w +X and -X pins (for pressure)
const uint16_t touchXMin = 875;           // Min value of X touch reading
const uint16_t touchXMax = 215;           // Max value of X touch reading
const uint16_t touchYmin = 330;           // Min value of Y touch reading
const uint16_t touchYMax = 800;           // Max value of Y touch reading
const uint16_t min_pressure = 50;         // Minimum pressure to register a touch
const uint16_t max_pressure = 300;       // Maximum pressure to register a touch
const int SCREEN_WIDTH = 480;             // screen width in pixels
const int SCREEN_HEIGHT = 320;            // screen height in pixels
const int SCREEN_ROTATION = 3;            // screen rotation in degrees
const int SCREEN_BRIGHTNESS = 255;        // screen brightness, 0-255
const int screen_update_delay_ms = 67;    // screen update delay in ms (about 15 fps)
const int touch_pressure_min = 10;        // minimum touch pressure to register a touch

// Serial communication settings
bool print_to_serial = true;            // print to serial port
const int SERIAL_BAUD = 19200;          // baud rate for serial communication
bool wait_for_serial = true;            // wait for serial connection before starting program
char swTxBuffer[256];                   // I2C serial transmit buffer
char swRxBuffer[256];                   // I2C serial receive buffer

// Analog (ADC) settings
const int16_t analog_resolution = 12;                          // 12 bit resolution
const int16_t max_analog = pow(2, analog_resolution)-1;        // Max analog resolution 2^(analog_resolution)-1 = 4095 for 12 bit, 2047 for 11 bit, 1023 for 10 bit
const int8_t analog_vref = 3.3;                                // Analog reference voltage, 5.0V for ArduinoMega2560      

bool heaters_on = false;                // Flag for heaters on/off

// Min and max values for the conversion functions
float min_pwm_output = 0.0;
float max_pwm_output = 255.0;
float min_flow_rate = 0.0;         // ml/min
float max_flow_rate = 1000.0;      // ml/min
float min_temp = 0.0;              // deg C
float max_temp = 100.0;            // deg C
float min_heat_flux = 0.0;         // W/m^2


// -----------------------------------------------------------------------------
// Conversions functions for outputs to values for later processing of LMS data
// -----------------------------------------------------------------------------

// Map function for float inputs
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Convert the flow rate from flow sensors
uint16_t ml_per_min_to_pwm_output(float flow_rate_ml_per_min) {
    // this equates to 2.55 ml/min per PWM output step
    return (uint16_t)(mapf(flow_rate_ml_per_min, min_flow_rate, max_flow_rate, min_pwm_output, max_pwm_output));
}

// Convert the temperature from the temperature sensor
uint16_t degC_to_pwm_output(float temp_deg_celcius) {
    // this equates to 0.255 degC per PWM output step
    return (uint16_t)(mapf(temp_deg_celcius, min_temp, max_temp, min_pwm_output, max_pwm_output));
}

// Convert the heat flux to a PWM output
uint16_t heat_flux_to_pwm_output(float heat_flux){
    // this equates to 51 mW/m^2 per PWM output step
    return (uint16_t)(mapf(heat_flux, min_heat_flux, max_heat_flux, min_pwm_output, max_pwm_output));
}




#endif