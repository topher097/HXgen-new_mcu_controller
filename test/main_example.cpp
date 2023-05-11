\

/*******************************************************************************
 * Purpose:  Example code for the I2C communication with Sensirion Liquid Flow
 *           Sensors based on the SF06 sensor chip.
 *
 *           This Code uses the SLF3S-1300F as an example. Adjust the sensor
 *           specific values as needed, see respective datasheet.
 *           Flow measurements are based on the water calibration
 *           with command: 0x3608. For IPA use 0x3615 instead.
 ******************************************************************************/
#define WIRINGTIMER_YIELD_DEFINE 1


#include <Arduino.h>
#include <Wire.h> 					// Arduino library for I2C
#include <Adafruit_MAX31855.h>		// Thermocouple library
#include <Timer.h>					// Wiring-timer library
#include <SoftwareWire.h>			// Software I2C library

#define ST(A) #A
#define STR(A) ST(A)

// -----------------------------------------------------------------------------
// Sensor specific settings and variables, adjust if needed:
// -----------------------------------------------------------------------------

const int ADDRESS = 0x08; // Address for SLF3x Liquid Flow Sensors
const float SCALE_FACTOR_FLOW = 500; // Scale Factor for flow rate measurement
const char *UNIT_FLOW = " ml/min"; // physical unit of the flow rate measurement
int ret;
int16_t signed_flow_value;
float scaled_flow_value;
byte sensor_flow_crc;
int ret2;
int16_t signed_flow_value2;
float scaled_flow_value2;
byte sensor_flow_crc2;


// -----------------------------------------------------------------------------
// Arduino setup routine, just runs once:
// -----------------------------------------------------------------------------
SoftwareWire wireInterface(A6, A5);
SoftwareWire wireInterface2(A4, A3);

void setup() {
	int ret;

	int bitEraser = 7;  // this is 111 in binary and is used to clear the bits
	int prescaler = 2;  // 1: 31000Hz, 2: 3906Hz, 3: 488Hz, 4: 122Hz, 5: 30Hz, 6: <20Hz
	TCCR1B &= bitEraser;  // clear the bits
	TCCR1B |= prescaler;  // set the bits to change freq of PWM for pin 12 and 11
	TCCR2B &= bitEraser;  // clear the bits
	TCCR2B |= prescaler;  // set the bits to change freq of PWM for pin 10 and 9
	TCCR3B &= bitEraser;  // clear the bits
	TCCR3B |= prescaler;  // set the bits to change freq of PWM for pin 5, 3, and 2
	TCCR4B &= bitEraser;  // clear the bits
	TCCR4B |= prescaler;  // set the bits to change freq of PWM for pin 8, 7, and 6

	wireInterface.begin();
	Serial.begin(19200); 	// initialize serial communication

	do {
		// Soft reset the sensor
		wireInterface.beginTransmission(0x00);
		wireInterface.write(0x06);
		ret = wireInterface.endTransmission();
		if (ret != 0) {
		Serial.println("Error while sending soft reset command to first sensor, retrying...");
		delay(500); 			// wait long enough for chip reset to complete
		}
	} while (ret != 0);

	int ret2;
	wireInterface2.begin();
	do {
		// Soft reset the sensor
		wireInterface2.beginTransmission(0x00);
		wireInterface2.write(0x06);
		ret2 = wireInterface2.endTransmission();
		if (ret2 != 0) {
		Serial.println("Error while sending soft reset command to second sensor, retrying...");
		delay(500); 			// wait long enough for chip reset to complete
		}
	} while (ret2 != 0);

	delay(50); 				// wait long enough for chip reset to complete

	// Set the sensor to continues measurement mode
	wireInterface.beginTransmission(ADDRESS);
	wireInterface.write(0x36);
	wireInterface.write(0x08);
	ret = wireInterface.endTransmission();
	if (ret != 0) {
		Serial.println("Error while sending start measurement command to the first sensor, retrying...");
	}

	// Set the sensor to continues measurement mode
	wireInterface2.beginTransmission(ADDRESS);
	wireInterface2.write(0x36);
	wireInterface2.write(0x08);
	ret = wireInterface2.endTransmission();
	if (ret != 0) {
		Serial.println("Error while sending start measurement command to the second sensor, retrying...");
	}

	// Print the status for each sensor
	wireInterface.printStatus(Serial);
	wireInterface2.printStatus(Serial);

}

// -----------------------------------------------------------------------------
// The Arduino loop routine runs over and over again forever:
// -----------------------------------------------------------------------------
void loop() {
	int qty = wireInterface.requestFrom(ADDRESS, 3);		// qty is the number of bytes received (should be 3), if not, there is an error
	//Serial.print(qty); Serial.print(", ");
	if (qty < 3) {
		Serial.println("Error while reading flow measurement");
	}
	else {
		signed_flow_value  = wireInterface.read() << 8; // read the MSB from the sensor
		signed_flow_value |= wireInterface.read();      // read the LSB from the sensor
		sensor_flow_crc    = wireInterface.read();

		scaled_flow_value = ((float) signed_flow_value) / SCALE_FACTOR_FLOW;
		//Serial.print("Sensor 1: ");
		Serial.print(scaled_flow_value); 
		// Serial.print(" ");
		// Serial.print(UNIT_FLOW);
		Serial.print(",        ");
	}

	int qty2 = wireInterface2.requestFrom(ADDRESS, 3);
	//Serial.print(qty2); Serial.print(", ");
	if (qty2 < 3) {
		Serial.println("Error while reading flow measurement");
	}
	else {
		signed_flow_value2  = wireInterface2.read() << 8; // read the MSB from the sensor
		signed_flow_value2 |= wireInterface2.read();      // read the LSB from the sensor
		sensor_flow_crc2    = wireInterface2.read();

		scaled_flow_value2 = ((float) signed_flow_value2) / SCALE_FACTOR_FLOW;
		//Serial.print("Sensor 2: ");
		Serial.print(scaled_flow_value2); 
		// Serial.print(" ");
		// Serial.println(UNIT_FLOW);
		Serial.println();
	}
  	
	delay(25); // milliseconds delay between reads (for demo purposes)
}
