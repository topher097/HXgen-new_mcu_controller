#ifndef FLOW_SENSOR_H
#define FLOW_SENSOR_H

/*
	Includes the methods to measure, calibrate, and calculate the flow rate of the HFE-7100 fluid
	Use the IPA flow calibration as the fluid flow calibration, it's close enough and all we need is qualitative data not necessarily quantitative data
	Using the SF06 > example 19 and 15 as the base of this code
*/

#pragma once
#include <Arduino.h>		// Arduino library for Arduino specific function
#include <Wire.h>
#include <config.h>			// Include the config.h header file


// Flow Sensor class
class FlowSensor {
public:
	FlowSensor(const String &name, TwoWire *wire): 
				sensor_name(name), _wire(wire){};
	
	void begin( void );
	void soft_reset( void );
	void measure_flow( void );
	void calibrate_flow( void );
	void set_continuous_mode( void );
	float get_flowrate( void );

	~FlowSensor(){};


	// Getter methods
	String get_name() const { 
		return sensor_name; 
	}

	TwoWire *get_bus() const { 
		return _wire; 
	}	

	volatile float scaled_flow_value = 0;	// This is the value of the flow rate in mL/min (signed and scaled)
	
private:
	volatile int ret;
	const String sensor_name;
	TwoWire *_wire;
	//SoftwareWire *_wire;
	volatile uint16_t sensor_flow_value;
	volatile byte aux_crc;
	volatile uint16_t aux_value;
	volatile byte sensor_flow_crc;
	volatile int16_t signed_flow_value;

};

#endif