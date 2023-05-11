// #define USBCON

// // Library includes
// #include <Arduino.h>
// #include <Wire.h>
// #include <Adafruit_MAX31855.h>
// #include <QuickPID.h>
// // #include <string>

// // Local includes
// // #include "flow_sensor.cpp"
// #include "config.h"

// // Inlet temperature sensor setup
// Adafruit_MAX31855 thermocouple(INLET_FLUID_THERMOCOUPLE_CS_PIN);   // Hardware SPI implementation

// // Timer objects
// IntervalTimer blink_led_timer;
// IntervalTimer update_screen_timer;
// IntervalTimer measure_flow_timer;

// // PID object for rope heater (https://github.com/Dlloydev/QuickPID/blob/master/examples/PID_RelayOutput/PID_RelayOutput.ino)
// QuickPID ropePID(&inlet_fluid_temp_measured, &rope_control_output, &inlet_fluid_temp_setpoint, 
// 				 rope_Kp, rope_Ki, rope_Kd,
// 				 QuickPID::pMode::pOnError,
// 				 QuickPID::dMode::dOnMeas,
// 				 QuickPID::iAwMode::iAwClamp,
// 				 QuickPID::Action::direct);

// // Create FlowSensor objects for the inlet and outlet sensors
// std::string inlet_string = "inlet";
// std::string outlet_string = "outlet";
// FlowSensor inlet_sensor(&inlet_string, &Wire);
// FlowSensor outlet_sensor(&outlet_string, &Wire1);

// // Blink the status LED (timer object)
// void blink_status_led() {
// 	if (led_state == LOW) {led_state = HIGH;} 
// 	else {led_state = LOW;}
// 	digitalWrite(STATUS_LED_BLINK_PIN, led_state);
// }

// // Measure the flow rate and temp (timer object)
// void measure_flow() {
// 	// Measure the flow rate
// 	inlet_sensor.measure_flow();
// 	outlet_sensor.measure_flow();

// 	// Print the flow rate and temp to the serial monitor
// 	Serial.printf("%s: %f %s\n", inlet_sensor.sensor_name, inlet_sensor.scaled_flow_value, UNIT_FLOW);
// 	Serial.printf("%s: %f %s\n", inlet_sensor.sensor_name, inlet_sensor.scaled_flow_value, UNIT_FLOW);
// }

// // Update the screen (timer object)
// void update_screen() {
// 	// Update the screen
// }

// // Setup routine
// void setup() {
// 	// Initialize the serial communication
//     Serial.begin(SERIAL_BAUD_RATE);
// 	if (wait_for_serial){while (!Serial) {}	}			// wait for serial port to connect. Needed for native USB port only
	
// 	// Setup the pins for the Teensy input and output
// 	pinMode(HEATER_BLOCK_RELAY_CONTROL_PIN, OUTPUT);
// 	pinMode(ROPE_HEATER_RELAY_CONTROL_PIN, OUTPUT);
// 	pinMode(VALVE_POTENTIOMETER_PIN, INPUT);
// 	pinMode(UPSTREAM_PRESSURE_SENSOR_PIN, INPUT);
// 	pinMode(DOWNSTREAM_PRESSURE_SENSOR_PIN, INPUT);
// 	pinMode(STATUS_LED_BLINK_PIN, OUTPUT);

// 	// // PID control setup
// 	// ropePID.SetOutputLimits(0, window_size_ms);
// 	// ropePID.SetSampleTimeUs(window_size_ms * 1000);
// 	// ropePID.SetMode(QuickPID::Control::automatic);

// 	// // Init the timers
// 	// blink_led_timer.begin(blink_status_led, blink_delay_ms * 1000);
// 	// blink_led_timer.priority(1);
// 	// measure_flow_timer.begin(measure_flow, measure_flow_delay_ms * 1000);
// 	// measure_flow_timer.priority(2);
// 	// update_screen_timer.begin(update_screen, screen_update_delay_ms * 1000);
// 	// update_screen_timer.priority(3);

// 	// // Setup the I2C busses for each sensor. Inlet sensor is on Wire and outlet sensor is on Wire1
//     // Wire.setSDA(INLET_FLOW_SENSOR_SDA);
//     // Wire.setSCL(INLET_FLOW_SENSOR_SCL);
//     // Wire.begin();
//     // Wire1.setSDA(OUTLET_FLOW_SENSOR_SDA);
//     // Wire1.setSCL(OUTLET_FLOW_SENSOR_SCL);
//     // Wire1.begin();

// 	// // Soft reset the inlet and outlet sensor
// 	// inlet_sensor.soft_reset();
// 	// outlet_sensor.soft_reset();



// }

// void loop() {
// 	// --------------------------- Control the rope heater PID loop --------------------------- 
// 	// First read the temperature of the inlet fluid (in Celsius since the setpoint is in Celsius)
// 	inlet_fluid_temp_measured = thermocouple.readCelsius();
	
// 	// // Run the PID loop
// 	// // rope_control_output is the TIME in milliseconds that the rope heater should be on, this is calculated in the PID controller
// 	// unsigned long ms_now = millis();
// 	// if (ropePID.Compute()) {window_start_time = ms_now;}		// Reset the window start time when the PID compute is done
// 	// if (!relay_status && rope_control_output > (ms_now - window_start_time)) {
// 	// 	if (ms_now > next_switch_time){
// 	// 		relay_status = true;
// 	// 		next_switch_time = ms_now + debounce_delay_ms;
// 	// 		digitalWrite(ROPE_HEATER_RELAY_CONTROL_PIN, HIGH);
// 	// 	}
// 	// } else if (relay_status && rope_control_output < (ms_now - window_start_time)) {
// 	// 	if (ms_now > next_switch_time){
// 	// 		relay_status = false;
// 	// 		next_switch_time = ms_now + debounce_delay_ms;
// 	// 		digitalWrite(ROPE_HEATER_RELAY_CONTROL_PIN, LOW);
// 	// 	}
// 	// }

// 	// // --------------------------- Measure the flow rate and temperature of inlet and outlet fluid ---------------------------
// 	// // Measure the flow rate and temperature for the inlet sensor
// 	// inlet_sensor.measure_flow();
	
// 	// // Measure the flow rate and temperature for the outlet sensor
// 	// outlet_sensor.measure_flow();
// 	// Serial.printf("%s: %f %s", outlet_sensor.sensor_name, outlet_sensor.scaled_flow_value, UNIT_FLOW);


// }