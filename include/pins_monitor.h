#ifndef _PINS_MONITOR_H
#define _PINS_MONITOR_H

#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Definitions for Monitor Teensy 4.1:
// -----------------------------------------------------------------------------

// Thermistor input pins
/* HEATER BLOCK THERMISTOR NUMBERINGS
inlet -----------> outlet
2   4   6   8   10   12  
1   3   5   7   9    11


        13   14
*/
const uint8_t THERMISTOR_1_INPUT_PIN = A1;
const uint8_t THERMISTOR_2_INPUT_PIN = A7;
const uint8_t THERMISTOR_3_INPUT_PIN = A0;
const uint8_t THERMISTOR_4_INPUT_PIN = A6;
const uint8_t THERMISTOR_5_INPUT_PIN = A17;
const uint8_t THERMISTOR_6_INPUT_PIN = A5;
const uint8_t THERMISTOR_7_INPUT_PIN = A16;
const uint8_t THERMISTOR_8_INPUT_PIN = A4;
const uint8_t THERMISTOR_9_INPUT_PIN = A15;
const uint8_t THERMISTOR_10_INPUT_PIN = A3;
const uint8_t THERMISTOR_11_INPUT_PIN = A14;
const uint8_t THERMISTOR_12_INPUT_PIN = A2;
const uint8_t THERMISTOR_13_INPUT_PIN = A8; 
const uint8_t THERMISTOR_14_INPUT_PIN = A9; 

// Thermistor pins as an array
const uint8_t THERMISTOR_PINS[] = {
  THERMISTOR_1_INPUT_PIN,
  THERMISTOR_2_INPUT_PIN,
  THERMISTOR_3_INPUT_PIN,
  THERMISTOR_4_INPUT_PIN,
  THERMISTOR_5_INPUT_PIN,
  THERMISTOR_6_INPUT_PIN,
  THERMISTOR_7_INPUT_PIN,
  THERMISTOR_8_INPUT_PIN,
  THERMISTOR_9_INPUT_PIN,
  THERMISTOR_10_INPUT_PIN,
  THERMISTOR_11_INPUT_PIN,
  THERMISTOR_12_INPUT_PIN,
  THERMISTOR_13_INPUT_PIN,
  THERMISTOR_14_INPUT_PIN
};

// Status LED pins, synced with Driver and MEGA Status LED
const int STATUS_PIN = 32;  // External status LED pin

// Mega measure flow trigger pin
const uint8_t MEGA_MEASURE_FLOW_TRIGGER_PIN = 6;  // External status LED pin

// SPI Pins (hardware SPI)
uint8_t SPI_MOSI = MOSI;
uint8_t SPI_MISO = MISO;
uint8_t SPI_SCK = SCK;
uint8_t INLET_FLUID_THERMOCOUPLE_CS_PIN = 37;

// Heater control pins
uint8_t HEATER_BLOCK_RELAY_CONTROL_PIN = 4;       // PWM capable necessary
uint8_t ROPE_HEATER_RELAY_CONTROL_PIN = 3;        // PWM capable, not used in code tho

// TFT display pins (use SPI mode)
uint8_t TFT_CS = 10;          // Chip select control pin for hardware SPI
uint8_t TFT_DC = 30;          // Data/command pin
uint8_t TFT_RST = 9;
uint8_t TOUCH_YP = A12;       // Must be analog pin
uint8_t TOUCH_XP = A13;       // Must be analog pin
uint8_t TOUCH_YM = 28;        // Can be digital pin
uint8_t TOUCH_XM = 29;        // Can be digital pin

#endif