#ifndef _PINS_DRIVER_H
#define _PINS_DRIVER_H

#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Definitions for Driver Teensy 4.0:
// -----------------------------------------------------------------------------

// Read sensors trigger pin
const uint8_t READ_SENSORS_TRIGGER_PIN = 2;     // Connects to Monitor teensy pin 6

// Status LED pin
const uint8_t STATUS_LED_BLINK_PIN = 14;            // Status LED pin

#endif