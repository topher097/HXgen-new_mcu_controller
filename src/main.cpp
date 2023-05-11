#define WIRINGTIMER_YIELD_DEFINE 1


// Library includes
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_HX8357.h>		// TFT display library
#include <Adafruit_GFX.h>			// GFX library
#include <Adafruit_MAX31855.h>		// Thermocouple library
#include <timer.h>					// Timer library
#include <QuickPID.h>				// PID library
#include <SoftwareWire.h>			// Software I2C library
#include <eRCaGuy_NewAnalogRead.h>	// NewAnalogRead library, artificaially increases ADC resolution
#include <GFX_PrettyMenu_Objects.h>	// PrettyMenu library

// Local includes
#include "flow_sensor.cpp"
#include "flow_valve.cpp"
#include "config.h"
//#include "GFX_PrettyMenu.h"

// Init the screen and touchscreen objects
Adafruit_TFTLCD tft(TFT_CS, TFT_DC, TFT_WR, TFT_RD, TFT_RESET);
TouchScreen touch_screen(TOUCH_XP, TOUCH_YP, TOUCH_XM, TOUCH_YM, screen_touch_resistance);

// Page objects
Button button_add_heat_flux(&tft, &touch_screen);
Button button_subtract_heat_flux(&tft, &touch_screen);
Button button_add_inlet_temp(&tft, &touch_screen);
Button button_subtract_inlet_temp(&tft, &touch_screen);
Button heaters_on_off_button(&tft, &touch_screen);
Button buttons[] = {button_add_heat_flux,
					button_subtract_heat_flux, 
					button_add_inlet_temp, 
					button_subtract_inlet_temp, 
					heaters_on_off_button};

Page main_page(&tft, &touch_screen);

// Callbacks for the buttons
void add_heat_flux_callback(){
	// Add 1 to the heat flux
	heat_flux += 1;
	// Limit the heat flux to the max
	if (heat_flux > max_heat_flux){
		heat_flux = max_heat_flux;
	}
};

void subtract_heat_flux_callback(){
	// Subtract 1 from the heat flux
	heat_flux -= 1;
	// Limit the heat flux to the min
	if (heat_flux < min_heat_flux){
		heat_flux = min_heat_flux;
	}
};

void add_inlet_temp_callback(){
	// Add 1 to the inlet temp
	inlet_fluid_temp_setpoint += 1;
	// Limit the inlet temp to the max
	if (inlet_fluid_temp_setpoint > inlet_fluid_temp_max){
		inlet_fluid_temp_setpoint = inlet_fluid_temp_max;
	}
};

void subtract_inlet_temp_callback(){
	// Subtract 1 from the inlet temp
	inlet_fluid_temp_setpoint -= 1;
	// Limit the inlet temp to the min
	if (inlet_fluid_temp_setpoint < inlet_fluid_temp_min){
		inlet_fluid_temp_setpoint = inlet_fluid_temp_min;
	}
};

void heaters_on_off_callback(){
	// Toggle the heaters on/off
	if (heaters_on){
		heaters_on = false;
	}
	else{
		heaters_on = true;
	}
};


void init_buttons(){
	// Calculate the button x center positions given evenly spaced across screen
	uint8_t math_button_width = tft.width() / 3;	// Three buttons horizontally
	uint8_t math_button_height = tft.height() / 6;
	uint8_t math_button_x_center_positions[3];
	for (int i=0; i<3; i++){
		math_button_x_center_positions[i] = math_button_width/2 + i*math_button_width;
	}
	uint8_t math_button_y_center_positions[2];		// One button at 3/6 and another at 5/6
	for (int i=0; i<2; i++){
		math_button_y_center_positions[i] = math_button_height/2 + (i+3)*math_button_height;
	}
	String add = "+";
	String minus = "-";
	String start_stop = "Start/Stop";
	uint16_t add_button_color = GREEN;
	uint16_t minus_button_color = RED;
	uint16_t start_stop_button_color = BLUE;
	uint16_t border_color = BLACK;
	uint16_t text_color = BLACK;
	uint8_t border_thickness = 1;
	uint8_t corner_radius = 3;
	uint8_t text_size = 3;
	uint8_t border_padding = 3;
	
	// Add Heat Flux Button
	button_add_heat_flux.init_button_center_coords(math_button_x_center_positions[0], math_button_y_center_positions[0], 
												   math_button_width, math_button_height,
												   add_button_color, border_color, border_thickness, corner_radius, border_padding, text_color, text_size, &add);
	button_add_heat_flux.set_min_max_pressure(min_pressure, max_pressure);
	button_add_heat_flux.set_min_max_x_y_values(touchXMin, touchXMax, touchYmin, touchYMax);
	button_add_heat_flux.attach_function_callback(add_heat_flux_callback);

	// Subtract Heat Flux Button
	button_subtract_heat_flux.init_button_center_coords(math_button_x_center_positions[0], math_button_y_center_positions[1], 
												   math_button_width, math_button_height,
												   minus_button_color, border_color, border_thickness, corner_radius, border_padding, text_color, text_size, &minus);
	button_subtract_heat_flux.set_min_max_pressure(min_pressure, max_pressure);
	button_subtract_heat_flux.set_min_max_x_y_values(touchXMin, touchXMax, touchYmin, touchYMax);
	button_subtract_heat_flux.attach_function_callback(subtract_heat_flux_callback);

	// Add Inlet Temp Button
	button_add_inlet_temp.init_button_center_coords(math_button_x_center_positions[1], math_button_y_center_positions[0], 
												   math_button_width, math_button_height,
												   add_button_color, border_color, border_thickness, corner_radius, border_padding, text_color, text_size, &add);
	button_add_inlet_temp.set_min_max_pressure(min_pressure, max_pressure);
	button_add_inlet_temp.set_min_max_x_y_values(touchXMin, touchXMax, touchYmin, touchYMax);
	button_add_inlet_temp.attach_function_callback(add_inlet_temp_callback);

	// Subtract Inlet Temp Button
	button_subtract_inlet_temp.init_button_center_coords(math_button_x_center_positions[1], math_button_y_center_positions[1], 
												   math_button_width, math_button_height,
												   minus_button_color, border_color, border_thickness, corner_radius, border_padding, text_color, text_size, &minus);
	button_subtract_inlet_temp.set_min_max_pressure(min_pressure, max_pressure);
	button_subtract_inlet_temp.set_min_max_x_y_values(touchXMin, touchXMax, touchYmin, touchYMax);
	button_subtract_inlet_temp.attach_function_callback(subtract_inlet_temp_callback);

	// Start/Stop Button
	heaters_on_off_button.init_button_center_coords(math_button_x_center_positions[2], math_button_y_center_positions[0], 
												   math_button_width, math_button_height,
												   start_stop_button_color, RED, border_thickness, corner_radius, border_padding, text_color, text_size, &start_stop);
	heaters_on_off_button.set_min_max_pressure(min_pressure, max_pressure);
	heaters_on_off_button.set_min_max_x_y_values(touchXMin, touchXMax, touchYmin, touchYMax);
	heaters_on_off_button.attach_function_callback(heaters_on_off_callback);
};

void init_page(){
	main_page.add_buttons(buttons, 5);

};


// Inlet temperature sensor setup
Adafruit_MAX31855 thermocouple(INLET_FLUID_THERMOCOUPLE_CS_PIN);   // Hardware SPI implementation

// PID object for rope heater (https://github.com/Dlloydev/QuickPID/blob/master/examples/PID_RelayOutput/PID_RelayOutput.ino)
QuickPID ropePID(&inlet_fluid_temp_measured, &rope_control_output, &inlet_fluid_temp_setpoint, 
				 rope_Kp, rope_Ki, rope_Kd,
				 QuickPID::pMode::pOnError,
				 QuickPID::dMode::dOnMeas,
				 QuickPID::iAwMode::iAwClamp,
				 QuickPID::Action::direct);

// Create FlowSensor objects for the inlet and outlet sensors
const String inlet_string = "inlet";
const String outlet_string = "outlet";
FlowSensor inlet_sensor(inlet_string, INLET_FLOW_SENSOR_SDA, INLET_FLOW_SENSOR_SCL);
FlowSensor outlet_sensor(outlet_string, OUTLET_FLOW_SENSOR_SDA, OUTLET_FLOW_SENSOR_SCL);
String temp;		// Temporary string for printing to serial monitor

// Create FlowValve objects for the inlet valve flow measurement
FlowValve inlet_valve(&adc);		// Send the adc object to the FlowValve class for reading pins

// Create timers
Timer statusLEDTimer;
Timer measureSensorsAndUpdateOutputTimer;
Timer updateScreenTimer;
uint16_t measure_count = 0;


// Blink the status LED (callback function for the timer)
void blink_status_led() {
	// Blink status LED
	Serial.println("BLINK"); 
	int blinks = 3;
	int blinking_ms = blink_delay_ms / (10 * blinks);
	for (int i = 0; i < blinks; i++) {
		digitalWrite(STATUS_LED_PIN, HIGH);
		delay(blinking_ms);
		digitalWrite(STATUS_LED_PIN, LOW);
		delay(blinking_ms);
	}
}


// Measure the flow sensors (callback function for the timer)
void measure_sensors_and_update_output() {
	if (print_to_serial) {
		Serial.print("Measure count: "); Serial.print(measure_count); Serial.print(",     ");
		measure_count++;
	}
	// --------------------------------- FLOW SENSORS ---------------------------------
	inlet_sensor.measure_flow();
	outlet_sensor.measure_flow();

	// Print the flow rate and temp to the serial monitor
	if (print_to_serial) {
		temp = inlet_sensor.get_name() + ": " + String(inlet_sensor.scaled_flow_value) + " " + String(UNIT_FLOW);
		Serial.print(temp); Serial.print(",     ");
		temp = outlet_sensor.get_name() + ": " + String(outlet_sensor.scaled_flow_value) + " " + String(UNIT_FLOW);
		Serial.print(temp); Serial.print(",     ");
	}

	// --------------------------------- VALVE FLOW CALC ---------------------------------
	// Run the measurement steps for the inlet valve
	inlet_valve.measure();

	// Print the flow rate to the serial monitor
	if (print_to_serial) {
		temp = "Valve: " + String(inlet_valve.inlet_flow_rate) + " " + String(UNIT_FLOW);
		Serial.print(temp);
		Serial.println();
	}

	// --------------------------------- UPDATE THE OUTPUT SIGNALS ---------------------------------
	// Update the inlet and outlet flow sensor output via the PWM pins
	analogWrite(INLET_FLOW_SENSOR_OUTPUT_PIN, ml_per_min_to_pwm_output(inlet_sensor.scaled_flow_value));
	analogWrite(OUTLET_FLOW_SENSOR_OUTPUT_PIN, ml_per_min_to_pwm_output(outlet_sensor.scaled_flow_value));

	// Update the inlet valve flow output via the PWM pin
	analogWrite(INLET_FLOW_VALVE_OUTPUT_PIN, ml_per_min_to_pwm_output(inlet_valve.inlet_flow_rate));

	// Update the inlet flow temperature output via the PWM pin
	analogWrite(INLET_FLOW_TEMP_OUTPUT_PIN, degC_to_pwm_output(inlet_fluid_temp_measured));
	
	// Updated the heat flux output via the PWM pin (so we can see the heat flux put into the system in analysis from LMS recording)
	analogWrite(HEATER_FLUX_OUTPUT_PIN, heat_flux_to_pwm_output(heat_flux));
}


// Update the screen with relavent information (callback function for the timer)
void udpate_screen() {

}


void setup() {
	// Initialize the serial communication
    Serial.begin(SERIAL_BAUD);
	if (wait_for_serial){while (!Serial) {}	}			// wait for serial port to connect. Needed for native USB port only

	// Change frequency of output PWM pins (for output to digital BNC connectors)
	int bitEraser = 7;  	// this is 111 in binary and is used to clear the bits
	int prescaler = 2;  	// 1: 31000Hz, 2: 3906Hz, 3: 488Hz, 4: 122Hz, 5: 30Hz, 6: <20Hz
	TCCR1B &= bitEraser;  	// clear the bits
	TCCR1B |= prescaler;  	// set the bits to change freq of PWM for pin 12 and 11
	TCCR2B &= bitEraser;  	// clear the bits
	TCCR2B |= prescaler;  	// set the bits to change freq of PWM for pin 10 and 9
	TCCR3B &= bitEraser;  	// clear the bits
	TCCR3B |= prescaler;  	// set the bits to change freq of PWM for pin 5, 3, and 2
	TCCR4B &= bitEraser;  	// clear the bits
	TCCR4B |= prescaler;  	// set the bits to change freq of PWM for pin 8, 7, and 6

	// Set the ADC settings
	adc.setADCSpeed(ADC_FAST);						// Set the ADC speed 
	adc.setBitsOfResolution(analog_resolution);		// Set the ADC resolution to 12 bits rather than 10 bits
	adc.setNumSamplesToAvg(3);						// Set the number of samples to average to 5
	analogReference(analog_vref);					// Set the ADC reference voltage to 3.3V rather than 5V
	
	// Setup the pins for the Teensy input and output
	pinMode(HEATER_BLOCK_RELAY_CONTROL_PIN, OUTPUT);
	pinMode(ROPE_HEATER_RELAY_CONTROL_PIN, OUTPUT);
	pinMode(VALVE_POTENTIOMETER_PIN, INPUT);
	pinMode(UPSTREAM_PRESSURE_SENSOR_PIN, INPUT);
	pinMode(DOWNSTREAM_PRESSURE_SENSOR_PIN, INPUT);
	pinMode(INLET_FLOW_SENSOR_OUTPUT_PIN, OUTPUT);
	pinMode(OUTLET_FLOW_SENSOR_OUTPUT_PIN, OUTPUT);
	pinMode(INLET_FLOW_VALVE_OUTPUT_PIN, OUTPUT);
	pinMode(INLET_FLOW_TEMP_OUTPUT_PIN, OUTPUT);
	pinMode(HEATER_FLUX_OUTPUT_PIN, OUTPUT);
	pinMode(STATUS_LED_PIN, OUTPUT);

	// Setup the timers
	statusLEDTimer.setInterval(blink_delay_ms, -1);
	statusLEDTimer.setCallback(blink_status_led);
	statusLEDTimer.start();
	measureSensorsAndUpdateOutputTimer.setInterval(measurement_update_delay_ms, -1);
	measureSensorsAndUpdateOutputTimer.setCallback(measure_sensors_and_update_output);
	measureSensorsAndUpdateOutputTimer.start();
	updateScreenTimer.setInterval(screen_update_delay_ms, -1);
	updateScreenTimer.setCallback(udpate_screen);
	updateScreenTimer.start();

	// PID control setup
	ropePID.SetOutputLimits(0, window_size_ms);
	ropePID.SetSampleTimeUs(window_size_ms * 1000);
	ropePID.SetMode(QuickPID::Control::automatic);


	// Soft reset the inlet and outlet sensors
	inlet_sensor.soft_reset();
	outlet_sensor.soft_reset();
	inlet_sensor.set_continuous_mode();
	outlet_sensor.set_continuous_mode();

	// Get the SoftwareWire object information for the inlet and outlet sensors
	//inlet_sensor.printStatus(Serial);
	//outlet_sensor.printStatus(Serial);
	//digitalWrite(STATUS_LED_PIN, HIGH);

}

void loop() {
	// --------------------------- Update the timer objects ---------------------------
	statusLEDTimer.update();
	//measureSensorsAndUpdateOutputTimer.update();
	//updateScreenTimer.update();

	// --------------------------- Control the rope heater PID loop ---------------------------
	// Measure the temperature of the inlet fluid (thermocouple)			
	inlet_fluid_temp_measured = thermocouple.readCelsius(); 

	// Run the PID loop
	// rope_control_output is the TIME in milliseconds that the rope heater should be on, this is calculated in the PID controller
	// the control pins need to be PWM enabled for this to work
	unsigned long ms_now = millis();
	if (ropePID.Compute()) {window_start_time = ms_now;}		// Reset the window start time when the PID compute is done
	if (!relay_status && rope_control_output > (ms_now - window_start_time)) {
		if (ms_now > next_switch_time){
			relay_status = true;
			next_switch_time = ms_now + debounce_delay_ms;
			digitalWrite(ROPE_HEATER_RELAY_CONTROL_PIN, HIGH);
		}
	} else if (relay_status && rope_control_output < (ms_now - window_start_time)) {
		if (ms_now > next_switch_time){
			relay_status = false;
			next_switch_time = ms_now + debounce_delay_ms;
			digitalWrite(ROPE_HEATER_RELAY_CONTROL_PIN, LOW);
		}
	}

	// --------------------------- Control the heater block ---------------------------
	// Update the PWM output of the heater block pin to match the desired heat flux
	heat_flux_pwm = map(heat_flux, 0, max_heat_flux, 0, 255);
	analogWrite(HEATER_FLUX_OUTPUT_PIN, heat_flux_pwm);

	

}