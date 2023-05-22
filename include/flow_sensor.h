#ifndef FLOW_SENSOR_H
#define FLOW_SENSOR_H

/*
	Includes the methods to measure, calibrate, and calculate the flow rate of the HFE-7100 fluid
	Use the IPA flow calibration as the fluid flow calibration, it's close enough and all we need is qualitative data not necessarily quantitative data
	Using the SF06 > example 19 and 15 as the base of this code
*/

#pragma once
//#include <SoftwareWire.h>   // Software I2C library
#include <Arduino.h>		// Arduino library for Arduino specific functions
#include <Wire.h>
#include <config.h>			// Include the config.h header file
#include <i2c_driver.h>		
#include <i2c_driver_wire.h>	



// Flow Sensor class
class FlowSensor {
public:
	void begin( void );
	//void printStatus(Print& Ser);
	void soft_reset( void );
	void measure_flow( void );
	void calibrate_flow( void );
	void set_continuous_mode( void );

	//FlowSensor(const String &name, const int _sdaPin, const int _sclPin): sensor_name(name), SensorWire(_sdaPin, _sclPin){};
	FlowSensor(const String &name, I2CDriverWire *wire): sensor_name(name), _wire(wire){};
	~FlowSensor(){};

	String get_name() const { 
		return sensor_name; 
	}
	

	volatile float scaled_flow_value = 0;	
	
private:
	volatile int ret;
	const String sensor_name;
	//TwoWire *_wire;
	I2CDriverWire *_wire;
	volatile uint16_t sensor_flow_value;
	volatile byte aux_crc;
	volatile uint16_t aux_value;
	volatile byte sensor_flow_crc;
	volatile int16_t signed_flow_value;
};

#endif