#ifndef _PINS_VALVE_H_
#define _PINS_VALVE_H_

#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Definitions for arduin Teensy 4 valve reader:
// -----------------------------------------------------------------------------
uint8_t STATUS_PIN = 13;    // Built-in LED

uint8_t UPSTREAM_NEG_PIN = A6;   // Upstream negative output
uint8_t UPSTREAM_POS_PIN = A7;   // Upstream positive output
uint8_t DOWNSTREAM_NEG_PIN = A2; // Downstream negative output
uint8_t DOWNSTREAM_POS_PIN = A3; // Downstream positive output

uint8_t POTENTIOMETER_PIN = A1; // Potentiometer output for valve position

#endif