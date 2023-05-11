#ifndef PINS_TEENSY_H
#define PINS_TEENSY_H

// -----------------------------------------------------------------------------
// Pin Definitions:
// -----------------------------------------------------------------------------

// SPI Pins
const int SPI_MOSI = 11;
const int SPI_MISO = 12;
const int SPI_SCK = 13;

// Flow sensor I2C pins
const int INLET_FLOW_SENSOR_SCL = 19;
const int INLET_FLOW_SENSOR_SDA = 18;
const int OUTLET_FLOW_SENSOR_SCL = 17;
const int OUTLET_FLOW_SENSOR_SDA = 16;

// Heater block control pins
const int HEATER_BLOCK_RELAY_CONTROL_PIN = 15;

// Rope heater constrol pins
const int ROPE_HEATER_RELAY_CONTROL_PIN = 14;
const int INLET_FLUID_THERMOCOUPLE_CS_PIN = 37;

// Flow valve sensor pins
const int VALVE_POTENTIOMETER_PIN = 41;            // Pin A17
const int UPSTREAM_PRESSURE_SENSOR_PIN = 40;       // Pin A16
const int DOWNSTREAM_PRESSURE_SENSOR_PIN = 39;     // Pin A15

// TFT display pins
const int TFT_CS = 10;          // Chip select control pin for hardware SPI
const int TFT_DC = 9;           // Data/command pin
const int TOUCH_YP = 24;        // Must be analog pin, A10
const int TOUCH_XP = 25;        // Must be analog pin, A11
const int TOUCH_YM = 26;        // Can be digital pin
const int TOUCH_XM = 27;        // Can be digital pin

// OTHER PINS
const int STATUS_LED_BLINK_PIN = 13;  // Status LED pin

#endif