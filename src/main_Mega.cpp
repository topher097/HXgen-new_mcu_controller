/*
Main file for the arduino MEGA 2560 that:
	controls the inlet temperature via reading thermocouple and controlling the rope heater with PID loop, 
	reads the inlet/outlet fluid flow rate, 
	measures the inlet valve position/pressures to calculate inlet fluid flow rate,
	controls the 


*/





#define WIRINGTIMER_YIELD_DEFINE 1		// For the wiring-timer library


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
#include <pageManager.h>


// Local includes
#include "flow_sensor.cpp"
#include "flow_valve.cpp"
#include "config.h"
#include "pages/main_page.h"
#include "pages/piezo_page.h"
#include "pages/inlet_temp_page.h"


// ************** COMMENT-OUT USE_TFTLCD IF USING SPI MODE **************
#define USE_TFTLCD

// Init the screen and touchscreen objects
Adafruit_TFTLCD tftlcd(TFT_CS, TFT_DC, TFT_WR, TFT_RD, TFT_RESET);
TouchScreen touch_screen(TOUCH_XP, TOUCH_YP, TOUCH_XM, TOUCH_YM, screen_touch_resistance);

// Create the Simple_GFX object
Simple_GFX<decltype(tftlcd)> tft(&tftlcd);

// Init the PageManager object
PageManager<decltype(tftlcd)> page_manager;

// Init the Page objects
MainPage<decltype(tftlcd)> main_page(&page_manager, &tft, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);
//PiezoPage piezo_page(&page_manager, &tft, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);
//InletTempPage inlet_temp_page(&page_manager, &tft, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);



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
	Serial.println(F("BLINK")); 
	int blinks = 3;
	int blinking_ms = blink_delay_ms / (10 * blinks);
	for (int i = 0; i < blinks; i++) {
		digitalWrite(STATUS_PIN, HIGH);
		delay(blinking_ms);
		digitalWrite(STATUS_PIN, LOW);
		delay(blinking_ms);
	}
}


// Measure the flow sensors (callback function for the timer)
void measure_sensors_and_update_output() {
	if (print_to_serial) {
		Serial.print(F("Measure count: ")); Serial.print(measure_count); Serial.print(F(",     "));
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

	// // --------------------------------- UPDATE THE OUTPUT SIGNALS ---------------------------------
	// // Update the inlet and outlet flow sensor output via the PWM pins
	// analogWrite(INLET_FLOW_SENSOR_OUTPUT_PIN, ml_per_min_to_pwm_output(inlet_sensor.scaled_flow_value));
	// analogWrite(OUTLET_FLOW_SENSOR_OUTPUT_PIN, ml_per_min_to_pwm_output(outlet_sensor.scaled_flow_value));

	// // Update the inlet valve flow output via the PWM pin
	// analogWrite(INLET_FLOW_VALVE_OUTPUT_PIN, ml_per_min_to_pwm_output(inlet_valve.inlet_flow_rate));

	// // Update the inlet flow temperature output via the PWM pin
	// analogWrite(INLET_FLOW_TEMP_OUTPUT_PIN, degC_to_pwm_output(inlet_fluid_temp_measured));
	
	// // Updated the heat flux output via the PWM pin (so we can see the heat flux put into the system in analysis from LMS recording)
	// analogWrite(HEATER_FLUX_OUTPUT_PIN, heat_flux_to_pwm_output(heat_flux));
}


void read_data_mega_struct() {
	// Only need to read the enable and disable for heaters, everything else is data only sent to the monitor
	rope_heater_enable  = data_mega.rope_heater_enable;
	heater_block_enable = data_mega.heater_block_enable;
};

void write_data_mega_struct() {
	// Write values to the data_mega struct to then be sent to the Monitor and be used in the pages
	data_mega.heat_flux 					= heat_flux;
	data_mega.inlet_valve_ml_min     		= inlet_valve.inlet_flow_rate;
	data_mega.inlet_flow_sensor_ml_min 		= inlet_sensor.scaled_flow_value;
	data_mega.outlet_flow_sensor_ml_min 	= outlet_sensor.scaled_flow_value;
	data_mega.inlet_fluid_temp 				= inlet_fluid_temp_measured;
	data_mega.rope_heater_enable 			= rope_heater_enable;
	data_mega.heater_block_enable 			= heater_block_enable;
	data_mega.piezo_1_freq 					= piezo_1_freq;
	data_mega.piezo_2_freq 					= piezo_2_freq;
	data_mega.piezo_1_vpp 					= piezo_1_vpp;
	data_mega.piezo_2_vpp 					= piezo_2_vpp;
	data_mega.piezo_1_phase 				= piezo_1_phase;
	data_mega.piezo_2_phase 				= piezo_2_phase;
	data_mega.piezo_1_enable 				= piezo_1_enable;
	data_mega.piezo_2_enable 				= piezo_2_enable;
	data_mega.start_stop_recording 			= start_stop_recording;
};

// Update the screen with relavent information (callback function for the timer)
void update_screen() {
	// Update the data_mega struct
	write_data_mega_struct();

	// Update the screen
	page_manager.update_current_page();
}


void setup() {
	// Initialize the serial communication
    Serial.begin(SERIAL_BAUD);
	if (wait_for_serial){while (!Serial) {}	}			// wait for serial port to connect. Needed for native USB port only

	Serial.println(F("START Free RAM = "));
    Serial.print(freeMemory());

	write_data_mega_struct();

	// // Change frequency of output PWM pins (for output to digital BNC connectors)
	// int bitEraser = 7;  	// this is 111 in binary and is used to clear the bits
	// int prescaler = 2;  	// 1: 31000Hz, 2: 3906Hz, 3: 488Hz, 4: 122Hz, 5: 30Hz, 6: <20Hz
	// TCCR1B &= bitEraser;  	// clear the bits
	// TCCR1B |= prescaler;  	// set the bits to change freq of PWM for pin 12 and 11
	// TCCR2B &= bitEraser;  	// clear the bits
	// TCCR2B |= prescaler;  	// set the bits to change freq of PWM for pin 10 and 9
	// TCCR3B &= bitEraser;  	// clear the bits
	// TCCR3B |= prescaler;  	// set the bits to change freq of PWM for pin 5, 3, and 2
	// TCCR4B &= bitEraser;  	// clear the bits
	// TCCR4B |= prescaler;  	// set the bits to change freq of PWM for pin 8, 7, and 6

	// Serial.println("Finished setting up the PWM pin speeds");

	// Set the ADC settings
	adc.setADCSpeed(ADC_FAST);						// Set the ADC speed 
	adc.setBitsOfResolution(analog_resolution);		// Set the ADC resolution to 12 bits rather than 10 bits
	adc.setNumSamplesToAvg(3);						// Set the number of samples to average to 5
	analogReference(analog_vref);					// Set the ADC reference voltage to 3.3V rather than 5V

	//Serial.println(F("Finished setting up the ADC settings"));
	
	// Setup the pins for the Teensy input and output
	pinMode(HEATER_BLOCK_RELAY_CONTROL_PIN, OUTPUT);
	pinMode(ROPE_HEATER_RELAY_CONTROL_PIN, OUTPUT);
	pinMode(VALVE_POTENTIOMETER_PIN, INPUT);
	pinMode(UPSTREAM_PRESSURE_SENSOR_PIN, INPUT);
	pinMode(DOWNSTREAM_PRESSURE_SENSOR_PIN, INPUT);
	// pinMode(INLET_FLOW_SENSOR_OUTPUT_PIN, OUTPUT);
	// pinMode(OUTLET_FLOW_SENSOR_OUTPUT_PIN, OUTPUT);
	// pinMode(INLET_FLOW_VALVE_OUTPUT_PIN, OUTPUT);
	// pinMode(INLET_FLOW_TEMP_OUTPUT_PIN, OUTPUT);
	// pinMode(HEATER_FLUX_OUTPUT_PIN, OUTPUT);
	pinMode(STATUS_PIN, OUTPUT);

	//Serial.println(F("Finished setting up the Mega pins"));

	// Setup the timers
	statusLEDTimer.setInterval(blink_delay_ms, -1);
	statusLEDTimer.setCallback(blink_status_led);
	statusLEDTimer.start();
	measureSensorsAndUpdateOutputTimer.setInterval(measurement_update_delay_ms, -1);
	measureSensorsAndUpdateOutputTimer.setCallback(measure_sensors_and_update_output);
	measureSensorsAndUpdateOutputTimer.start();
	updateScreenTimer.setInterval(screen_update_delay_ms, -1);
	updateScreenTimer.setCallback(update_screen);
	updateScreenTimer.start();
	//Serial.println(F("Finished setting up the timers"));

	// PID control setup
	ropePID.SetOutputLimits(0, window_size_ms);
	ropePID.SetSampleTimeUs(window_size_ms * 1000);
	ropePID.SetMode(QuickPID::Control::automatic);
	//Serial.println(F("Finished setting up the PID controller"));

	// TFT setup
	#ifdef USE_TFTLCD	// TFT 8bit setup
		tft.reset();
		uint16_t identifier = tft.get()->readID();		// 33623 for HX8357D
		tft.begin_8bit(identifier);
	#else	// TFT SPI setup
		uint16_t SPI_SPEED = 40000000; 			// 40 MHz
		tft.reset();
		tft.begin_spi(SPI_SPEED);
	#endif
	tft.setRotation(1);
	tft.fillScreen(RED);


	// Setup the page manager
	// page_manager.add_page(MAIN_PAGE, &main_page);
	// Serial.println("Added the main page to manager");
	// page_manager.add_page(INLET_TEMP_PAGE, &inlet_temp_page);
	// Serial.println("Added the inlet temp page to manager");
	// //page_manager.add_page(HEATER_FLUX_PAGE, &heater_flux_page);
	// //page_manager.add_page(PIEZO_CONFIG_PAGE, &piezo_page);
	// page_manager.set_page(MAIN_PAGE);		// Set the first page to main page
	// Serial.println("Set the main page as the current page");

	// Serial.println("Finished setting up the page manager");

	main_page.initialize();


	

	// Soft reset the inlet and outlet sensors
	// inlet_sensor.soft_reset();
	// outlet_sensor.soft_reset();
	// inlet_sensor.set_continuous_mode();
	// outlet_sensor.set_continuous_mode();
	//Serial.println("Finished setting up the flow sensors");

	Serial.println(F("Setup complete"));

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
	//inlet_fluid_temp_measured = thermocouple.readCelsius(); 

	// Run the PID loop
	// rope_control_output is the TIME in milliseconds that the rope heater should be on, this is calculated in the PID controller
	// the control pins need to be PWM enabled for this to work
	// unsigned long ms_now = millis();
	// if (ropePID.Compute()) {window_start_time = ms_now;}		// Reset the window start time when the PID compute is done
	// if (!relay_status && rope_control_output > (ms_now - window_start_time)) {
	// 	if (ms_now > next_switch_time){
	// 		relay_status = true;
	// 		next_switch_time = ms_now + debounce_delay_ms;
	// 		digitalWrite(ROPE_HEATER_RELAY_CONTROL_PIN, HIGH);
	// 	}
	// } else if (relay_status && rope_control_output < (ms_now - window_start_time)) {
	// 	if (ms_now > next_switch_time){
	// 		relay_status = false;
	// 		next_switch_time = ms_now + debounce_delay_ms;
	// 		digitalWrite(ROPE_HEATER_RELAY_CONTROL_PIN, LOW);
	// 	}
	// }

	// // --------------------------- Control the heater block ---------------------------
	// // Update the PWM output of the heater block pin to match the desired heat flux
	// heat_flux_pwm = map(heat_flux, 0, max_heat_flux, 0, 255);
	// analogWrite(HEATER_FLUX_OUTPUT_PIN, heat_flux_pwm);
}