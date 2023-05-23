#ifndef PINS_MEGA_H
#define PINS_MEGA_H

#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin Definitions for arduin MEGA 2560:
// -----------------------------------------------------------------------------
// EasyTransfer pins
uint8_t ET_RX_PIN = 18;     // rx1
uint8_t ET_TX_PIN = 19;     // tx1

// Flow sensor I2C pins (using software I2C via Software I2C library)
uint8_t INLET_FLOW_SENSOR_SDA = A15;
uint8_t INLET_FLOW_SENSOR_SCL = A14;       
uint8_t OUTLET_FLOW_SENSOR_SDA = A13;
uint8_t OUTLET_FLOW_SENSOR_SCL = A12;

// Status LED pins
uint8_t STATUS_PIN = 13;    // Built-in LED


#endif