/*
Main file for the Teensy 4.1 that:
    measures all 14 thermistors,
    measures the 2 flow sensors using I2C,
    controls the TFT display,
    communicates with the Driver Teensy to send the signals to the piezo amplifier via EasyTransfer,
    logs all of the data to the SD card
*/

// ************** COMMENT-OUT USE_TFTLCD IF USING SPI MODE **************
//#define USE_TFTLCD

#define WIRINGTIMER_YIELD_DEFINE 1		// For the wiring-timer library


// Library includes
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_HX8357.h>		// TFT display library
#include <Adafruit_GFX.h>			// GFX library
#include <Adafruit_MAX31855.h>		// Thermocouple library
#include <timer.h>					// Timer library
#include <QuickPID.h>				// PID library
#include <pageManager.h>            // PageManager library, brings Simple_GFX and all other class objest into scope


// Local includes
#include "flow_sensor.cpp"
#include "config.h"
#include "pages/main_page.h"
#include "pages/piezo_page.h"
#include "pages/inlet_temp_page.h"




// Init the screen and touchscreen objects
Adafruit_HX8357 tftlcd = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
TouchScreen touch_screen(TOUCH_XP, TOUCH_YP, TOUCH_XM, TOUCH_YM, screen_touch_resistance);

// Create the Simple_GFX object
uint16_t tft_width = 480;       // Can set these to the dimensions of your screen
uint16_t tft_height = 320;     
Simple_GFX<decltype(tftlcd)> sim_gfx(&tftlcd, 1, tft_width, tft_height);

// Init the PageManager object
PageManager<decltype(tftlcd)> page_manager;

// Init the Page objects
MainPage<decltype(tftlcd)> main_page(&page_manager, &sim_gfx, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);
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
FlowSensor inlet_sensor(inlet_string, &Wire);
FlowSensor outlet_sensor(outlet_string, &Wire1);
String temp;		// Temporary string for printing to serial monitor

// Create timers
Timer statusLEDTimer;
Timer measureSensorsAndUpdateOutputTimer;
Timer updateScreenTimer;
uint16_t measure_count = 0;


// Blink the status LED (callback function for the timer)
void blink_status_led() {
	// Blink status LED
	//Serial.println(F("BLINK"));
    blink_led_state = !blink_led_state;
    digitalWrite(STATUS_PIN, blink_led_state);

	// int blinks = 3;
	// int blinking_ms = blink_delay_ms / (10 * blinks);
	// for (int i = 0; i < blinks; i++) {
	// 	digitalWrite(STATUS_PIN, HIGH);
	// 	delay(blinking_ms);
	// 	digitalWrite(STATUS_PIN, LOW);
	// 	delay(blinking_ms);
	// }
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

    // --------------------------------- HEATER BLOCK THERMOCOUPLES ---------------------------------

	// // --------------------------------- VALVE FLOW CALC ---------------------------------
	// // Run the measurement steps for the inlet valve
	// inlet_valve.measure();

	// // Print the flow rate to the serial monitor
	// if (print_to_serial) {
	// 	temp = "Valve: " + String(inlet_valve.inlet_flow_rate) + " " + String(UNIT_FLOW);
	// 	Serial.print(temp);
	// 	Serial.println();
	// }

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


void read_save_data_struct() {
	// Only need to read the enable and disable for heaters, everything else is data only sent to the monitor
	rope_heater_enable  = save_data.rope_heater_enable;
	heater_block_enable = save_data.heater_block_enable;
};

void write_data_save_struct() {
	// Write values to the save_data struct to then be sent to the Monitor and be used in the pages
	save_data.heat_flux 					= heat_flux;
	//save_data.inlet_valve_ml_min     		= inlet_valve.inlet_flow_rate;
	save_data.inlet_flow_sensor_ml_min 		= inlet_sensor.scaled_flow_value;
	save_data.outlet_flow_sensor_ml_min 	= outlet_sensor.scaled_flow_value;
	save_data.inlet_fluid_temp 				= inlet_fluid_temp_measured;
	save_data.rope_heater_enable 			= rope_heater_enable;
	save_data.heater_block_enable 			= heater_block_enable;
	save_data.piezo_1_freq 					= piezo_1_freq;
	save_data.piezo_2_freq 					= piezo_2_freq;
	save_data.piezo_1_vpp 					= piezo_1_vpp;
	save_data.piezo_2_vpp 					= piezo_2_vpp;
	save_data.piezo_1_phase 				= piezo_1_phase;
	save_data.piezo_2_phase 				= piezo_2_phase;
	save_data.piezo_1_enable 				= piezo_1_enable;
	save_data.piezo_2_enable 				= piezo_2_enable;
	save_data.start_stop_recording 			= start_stop_recording;
};

// Update the screen with relavent information (callback function for the timer)
void update_screen() {
	// Update the screen
	page_manager.update_current_page();
}


void setup() {
	// Initialize the serial communication
    Serial.begin(SERIAL_BAUD);
	if (wait_for_serial){while (!Serial) {}	}			// wait for serial port to connect. Needed for native USB port only

	write_data_save_struct();

    // Begin the Wire busses and SPI bus
    //uint32_t SPI_SPEED = 40000000;   // 40MHz
    Wire.begin();
    Wire1.begin();
    SPI.begin();

    // Check the Simple_GFX pointers are correct and compare the address of the tftlcs and TCTClass
    if (sim_gfx.check_tft_pointer_exists()){
        Serial.print("The address of tftlcd in main.cpp is: "); Serial.println((uintptr_t)&tftlcd, HEX);
        Serial.print("The address of TFTClass in sim_gfx is: "); Serial.println(sim_gfx.get_tft_address_as_int(), HEX);
    } else {
        Serial.println("The pointer to TFTClass is invalid in Simple_GFX object");
    }

    // Print the address to sim_gfx object
    Serial.print("The address of sim_gfx in main.cpp is: "); Serial.println((uintptr_t)&sim_gfx, HEX);
    main_page.get_sim_gfx_pointer_addr();


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
	analogReadResolution(analog_resolution);		// Set the ADC resolution to 16 bits
	analogReference(analog_vref);					// Set the ADC reference voltage to 3.3V rather than 5V

	//Serial.println(F("Finished setting up the ADC settings"));
	
	// Setup the pins for the Teensy input and output
	pinMode(HEATER_BLOCK_RELAY_CONTROL_PIN, OUTPUT);
	pinMode(ROPE_HEATER_RELAY_CONTROL_PIN, OUTPUT);
	pinMode(STATUS_PIN, OUTPUT);
    pinMode(THERMISTOR_1_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_2_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_3_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_4_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_5_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_6_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_7_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_8_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_9_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_10_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_11_INPUT_PIN, INPUT);
    pinMode(THERMISTOR_12_INPUT_PIN, INPUT);


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

	
	// sim_gfx.fillScreen(RED);
    // Serial.print(F("TFT screen size is "));
    // Serial.print(sim_gfx.width());
    // Serial.print(F("x"));
    // Serial.println(sim_gfx.height());
    // sim_gfx.fillRoundRect(3, 179, 114, 122, 5, BLACK);
    // //tft.fillRoundRect(0, 160, 120, 160, 10, WHITE);
    // delay(5000);
    // //Serial.println(F("Finished setting up the TFT screen"));


	// Setup the page manager
	page_manager.add_page(MAIN_PAGE, &main_page);
	Serial.println("Added the main page to manager");
	//page_manager.add_page(INLET_TEMP_PAGE, &inlet_temp_page);
	//Serial.println("Added the inlet temp page to manager");
	//page_manager.add_page(HEATER_FLUX_PAGE, &heater_flux_page);
	//page_manager.add_page(PIEZO_CONFIG_PAGE, &piezo_page);
	page_manager.set_page(MAIN_PAGE);		// Set the first page to main page
	Serial.println("Set the main page as the current page");

	// Serial.println("Finished setting up the page manager");

	//main_page.initialize();


	

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
	updateScreenTimer.update();
    
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