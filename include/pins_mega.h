#ifndef PINS_MEGA_H
#define PINS_MEGA_H

#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Definitions:
// -----------------------------------------------------------------------------

// SPI Pins (hardware SPI)
uint8_t SPI_MOSI = 51;
uint8_t SPI_MISO = 50;
uint8_t SPI_SCK = 52;
uint8_t INLET_FLUID_THERMOCOUPLE_CS_PIN = 25;

// Flow sensor I2C pins (using software I2C via Software I2C library)
uint8_t INLET_FLOW_SENSOR_SDA = A6;
uint8_t INLET_FLOW_SENSOR_SCL = A5;       
uint8_t OUTLET_FLOW_SENSOR_SDA = A4;
uint8_t OUTLET_FLOW_SENSOR_SCL = A3;

// Heater block control pins
uint8_t HEATER_BLOCK_RELAY_CONTROL_PIN = 11;       // PWM capable

// Rope heater constrol pins
uint8_t ROPE_HEATER_RELAY_CONTROL_PIN = 10;        // PWM capable, not used in code tho

// Flow valve sensor pins
uint8_t VALVE_POTENTIOMETER_PIN = A0;            // Pin A17
uint8_t UPSTREAM_PRESSURE_SENSOR_PIN = A1;       // Pin A16
uint8_t DOWNSTREAM_PRESSURE_SENSOR_PIN = A2;     // Pin A15

// Status LED pins
uint8_t STATUS_LED_PIN = 13;



// TFT display pins (use 8-bit control bc we can lol)
//   D0 connects to digital pin 22
//   D1 connects to digital pin 23
//   D2 connects to digital pin 24
//   D3 connects to digital pin 25
//   D4 connects to digital pin 26
//   D5 connects to digital pin 27
//   D6 connects to digital pin 28
//   D7 connects to digital pin 29
uint8_t TFT_CS = A15;         // Chip select control pin for hardware SPI
uint8_t TFT_DC = A14;         // Data/command pin
uint8_t TFT_WR = A13;         // Write strobe control pin
uint8_t TFT_RD = A12;         // Read strobe control pin
uint8_t TFT_RESET = 47;       // Reset pin 
uint8_t TOUCH_YP = A8;        // Must be analog pin, A10
uint8_t TOUCH_XP = A9;        // Must be analog pin, A11
uint8_t TOUCH_YM = 48;        // Can be digital pin
uint8_t TOUCH_XM = 49;        // Can be digital pin

// Analog/PWN Output Pins for BNC output (skip pin 4 (and 13) bc it's freq is different from the others)
uint8_t INLET_FLOW_SENSOR_OUTPUT_PIN = 5;       // PWM capable
uint8_t OUTLET_FLOW_SENSOR_OUTPUT_PIN = 6;      // PWM capable
uint8_t INLET_FLOW_VALVE_OUTPUT_PIN = 7;        // PWM capable
uint8_t INLET_FLOW_TEMP_OUTPUT_PIN = 8;         // PWM capable
uint8_t HEATER_FLUX_OUTPUT_PIN = 9;             // PWM capable

#endif