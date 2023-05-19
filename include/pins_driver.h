#ifndef _PINS_DRIVER_H
#define _PINS_DRIVER_H

#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Definitions for Driver Teensy 4.0:
// -----------------------------------------------------------------------------


// Thermistor input pins
const uint8_t THERMISTOR_1_INPUT_PIN = A0;
const uint8_t THERMISTOR_2_INPUT_PIN = A1;
const uint8_t THERMISTOR_3_INPUT_PIN = A2;
const uint8_t THERMISTOR_4_INPUT_PIN = A3;
const uint8_t THERMISTOR_5_INPUT_PIN = A4;
const uint8_t THERMISTOR_6_INPUT_PIN = A5;
const uint8_t THERMISTOR_7_INPUT_PIN = A6;
const uint8_t THERMISTOR_8_INPUT_PIN = A7;
const uint8_t THERMISTOR_9_INPUT_PIN = A8;
const uint8_t THERMISTOR_10_INPUT_PIN = A9;
const uint8_t THERMISTOR_11_INPUT_PIN = A10;
const uint8_t THERMISTOR_12_INPUT_PIN = A11;
const uint8_t THERMISTOR_13_INPUT_PIN = A12;
const uint8_t THERMISTOR_14_INPUT_PIN = A13;

// EasyTransfer RXTX pins
const uint8_t EASY_TRANSFER_RX_PIN = 15;        // RX3
const uint8_t EASY_TRANSFER_TX_PIN = 14;        // TX3

// Status LED pins, synced with Monitor and MEGA Status LED
const int STATUS_LED_BLINK_PIN = 14;  // Status LED pin

#endif