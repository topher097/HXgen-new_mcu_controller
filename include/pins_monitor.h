#ifndef _PINS_MONITOR_H
#define _PINS_MONITOR_H

#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Definitions for Monitor Teensy 4.1:
// -----------------------------------------------------------------------------
// Analog reference input pin (from the heater sensor board)
const uint8_t ANALOG_REF_PIN = A2;


// Thermistor input pins
const uint8_t THERMISTOR_1_INPUT_PIN = A9;
const uint8_t THERMISTOR_2_INPUT_PIN = A8;
const uint8_t THERMISTOR_3_INPUT_PIN = A7;
const uint8_t THERMISTOR_4_INPUT_PIN = A6;
const uint8_t THERMISTOR_5_INPUT_PIN = A1;
const uint8_t THERMISTOR_6_INPUT_PIN = A0;
const uint8_t THERMISTOR_7_INPUT_PIN = A17;
const uint8_t THERMISTOR_8_INPUT_PIN = A16;
const uint8_t THERMISTOR_9_INPUT_PIN = A15;
const uint8_t THERMISTOR_10_INPUT_PIN = A14;
const uint8_t THERMISTOR_11_INPUT_PIN = A10;
const uint8_t THERMISTOR_12_INPUT_PIN = A11;
// const uint8_t THERMISTOR_13_INPUT_PIN = A12;     // Not enough pins
// const uint8_t THERMISTOR_14_INPUT_PIN = A13;     // Not enough pins

// EasyTransfer RXTX pins
const uint8_t EASY_TRANSFER_RX_PIN = 34;        // RX8
const uint8_t EASY_TRANSFER_TX_PIN = 35;        // TX8

// Status LED pins, synced with Driver and MEGA Status LED
const int STATUS_PIN = 32;  // Status LED pin

// SPI Pins (hardware SPI)
uint8_t SPI_MOSI = MOSI;
uint8_t SPI_MISO = MISO;
uint8_t SPI_SCK = SCK;
uint8_t INLET_FLUID_THERMOCOUPLE_CS_PIN = 37;

// Flow sensor I2C pins (using software I2C via Software I2C library but using hardware pins for Wire and Wire1)
uint8_t INLET_FLOW_SENSOR_SDA = 18;         // Wire is for inlet sensor
uint8_t INLET_FLOW_SENSOR_SCL = 19;       
uint8_t OUTLET_FLOW_SENSOR_SDA = 17;        // Wire1 is for outlet sensor
uint8_t OUTLET_FLOW_SENSOR_SCL = 16;

// Heater block control pins
uint8_t HEATER_BLOCK_RELAY_CONTROL_PIN = 0;       // PWM capable

// Rope heater constrol pins
uint8_t ROPE_HEATER_RELAY_CONTROL_PIN = 1;        // PWM capable, not used in code tho

// TFT display pins (use SPI mode)
uint8_t TFT_CS = 10;          // Chip select control pin for hardware SPI
uint8_t TFT_DC = 30;          // Data/command pin
uint8_t TFT_RST = 9;
uint8_t TOUCH_YP = A12;       // Must be analog pin
uint8_t TOUCH_XP = A13;       // Must be analog pin
uint8_t TOUCH_YM = 28;        // Can be digital pin
uint8_t TOUCH_XM = 29;        // Can be digital pin



#endif