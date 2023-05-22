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
#include <Timer.h>					// Wiring-timer library
#include <SoftwareWire.h>			// Software I2C library
#include <EasyTransfer.h>

#include <config.h>
#include <flow_sensor.cpp>

#define ST(A) #A
#define STR(A) ST(A)

#define TO_MASTER_SERIAL Serial1		// Serial port to master (monitor teensy, using rx1 and tx1)

// Create FlowSensor objects for the inlet and outlet sensors
const String inlet_string = "inlet";
const String outlet_string = "outlet";
SoftwareWire inlet_wire(INLET_FLOW_SENSOR_SDA, INLET_FLOW_SENSOR_SCL);
SoftwareWire outlet_wire(OUTLET_FLOW_SENSOR_SDA, OUTLET_FLOW_SENSOR_SCL);
FlowSensor inlet_sensor(inlet_string, &inlet_wire);
FlowSensor outlet_sensor(outlet_string, &outlet_wire);
String temp;		// Temporary string for printing to serial monitor


// Create timer object for the blink function
Timer statusLEDTimer;
Timer flowMeasurementTimer;

// Easytransfer object
EasyTransfer ETout;

// Variables
uint16_t thermistor_13_raw;
uint16_t thermistor_14_raw;


// Blink the status LED (callback function for the timer)
void blink_status_led() {
    blink_led_state = !blink_led_state;
    digitalWrite(STATUS_PIN, blink_led_state);
}


void print_sensors_to_serial(){
	// Print the sensor values to the serial monitor
	Serial.print(millis()); Serial.print(", ");
	Serial.print("In: ");
	Serial.print(inlet_sensor.scaled_flow_value);
	Serial.print(" ml/min, Out: ");
	Serial.print(outlet_sensor.scaled_flow_value);
	Serial.print(" ml/min, T13: ");
	Serial.print(thermistor_13_temp);
	Serial.print(" C, T14: ");
	Serial.print(thermistor_13_temp);
	Serial.println(" C");
}


void sensor_measurement() {
	// Measure the inlet and outlet flow rates
	inlet_sensor.measure_flow();
	outlet_sensor.measure_flow();

	// Measure the thermistor values and convert to temp in C
	thermistor_13_raw = analogRead(THERMISTOR_13_PIN);
	thermistor_14_raw = analogRead(THERMISTOR_14_PIN);
	thermistor_13_temp = convert_thermistor_analog_to_celcius(thermistor_13_raw, analog_mega_max, analog_vref, thermistor_lower_R1);
	thermistor_14_temp = convert_thermistor_analog_to_celcius(thermistor_14_raw, analog_mega_max, analog_vref, thermistor_lower_R1);

	// Save the results to the struct
	mega_data.inlet_flow_sensor_ml_min = inlet_sensor.scaled_flow_value;
	mega_data.outlet_flow_sensor_ml_min = outlet_sensor.scaled_flow_value;
	mega_data.thermistor_13_temp = thermistor_13_temp;
	mega_data.thermistor_14_temp = thermistor_14_temp;

	// Send the data to the master
	digitalWrite(STATUS_PIN, HIGH);
	ETout.sendData();
	digitalWrite(STATUS_PIN, LOW);
	print_sensors_to_serial();
}


void setup() {
	ETout.begin(details(mega_data), &TO_MASTER_SERIAL);


	Serial.begin(19200); 	// initialize serial communication
	// while (!Serial) {
	// 	; // wait for serial port to connect. Needed for native USB port only
	// }

	Serial.println("Starting up...");

	inlet_sensor.begin();
	outlet_sensor.begin();
	inlet_sensor.soft_reset();
	outlet_sensor.soft_reset();
	inlet_sensor.set_continuous_mode();
	outlet_sensor.set_continuous_mode();

	// statusLEDTimer.setInterval(500, -1);
	// statusLEDTimer.setCallback(blink_status_led);
	// statusLEDTimer.start();
	flowMeasurementTimer.setInterval(measurement_update_delay_ms, -1);
	flowMeasurementTimer.setCallback(sensor_measurement);
	flowMeasurementTimer.start();

	
	analogReference(EXTERNAL);
}

// -----------------------------------------------------------------------------
// The Arduino loop routine runs over and over again forever:
// -----------------------------------------------------------------------------
void loop() {
	// Update the timers
	//statusLEDTimer.update();
	flowMeasurementTimer.update();

}
