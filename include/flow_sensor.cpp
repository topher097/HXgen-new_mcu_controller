#include <flow_sensor.h>


// Begin the wire interface for the sensor
void FlowSensor::begin(){
	_wire->begin();		// Uses the onboard I2C pins by default, as well as clock speed
}


// Soft reset function which takes the Wire object as an argument
void FlowSensor::soft_reset(){
	do {
		_wire->beginTransmission(0x00);
		_wire->write(0x06);
		ret = _wire->endTransmission(false);
		if (ret != 0) { // Error sending soft reset command, retry
			Serial.print(ret); Serial.print(", "); Serial.print(sensor_name); Serial.print(", ");
			Serial.println("Error during soft reset command, retrying...");
			delay(500); // wait long enough for chip reset to complete
		}
	} while (ret != 0);
	delay(50); // wait long enough for chip reset to complete

}

void FlowSensor::set_continuous_mode(){
	// Set the sensor to continues measurement mode
	// To perform a measurement, first send 0x3608 to switch to continuous
	// measurement mode (H20 calibration), then read 3x (2 bytes + 1 CRC byte) from the sensor.
	// To perform a IPA based measurement, send 0x3615 instead.
	// Check datasheet for available measurement commands.
	do {
		_wire->beginTransmission(FLOW_SENSOR_ADDRESS);
		_wire->write(MEASUREMENT_MODE_B1);
		_wire->write(MEASUREMENT_MODE_B2);
		ret = _wire->endTransmission();
		if (ret != 0) {
			Serial.print(ret); Serial.print(", Error during write measurement mode command for the "); Serial.print(sensor_name); Serial.print(" sensor, retrying...\n");
			delay(50);
		}
	} while (ret != 0);
}


// Measurement function which takes the Wire object as an argument and returns the corrected flow rate and temperature for the sensor on that Wire object
void FlowSensor::measure_flow(){
	delay(25); 
	int qty = _wire->requestFrom(FLOW_SENSOR_ADDRESS, 3);
	if (qty < 3) {
		String temp = String(qty) + ", Error while reading flow measurement for the " + sensor_name + " sensor, retrying...\n";
		Serial.println(temp);
	}
	else {
		//Serial.println("Reading measurement...");
		signed_flow_value  = _wire->read() << 8; // read the MSB from the sensor
		signed_flow_value |= _wire->read();      // read the LSB from the sensor
		sensor_flow_crc    = _wire->read();

		scaled_flow_value = ((float) signed_flow_value) / SCALE_FACTOR_FLOW;
	}	
};