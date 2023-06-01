/*
Main file for the Teensy 4.1 that:
    measures all 12 thermistors,
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
#include <Adafruit_HX8357.h>		// TFT display library
#include <Adafruit_MAX31855.h>		// Thermocouple library
#include <timer.h>					// Timer library
#include <QuickPID.h>				// PID library
#include <EasyTransfer.h>			// EasyTransfer library

// Local includes
#include "config.h"
#include "pins_monitor.h"

#define PC_COMMUNICATION Serial		// Serial port for PC Communications
String temp;						// Temporary string for printing to serial monitor

// Initialize the EasyTransfer object
EasyTransfer ETin_pc;				// EasyTransfer object for receiving data from the PC
EasyTransfer ETout_pc;				// EasyTransfer object for sending data to the PC
bool new_data = false;				// Flag for new data from the PC

// Setup time objects
elapsedMillis elapsed_millis;		// Milliseconds since the Teensy started
elapsedMicros elapsed_micros;		// Microseconds since the Teensy started

// Inlet temperature sensor setup
Adafruit_MAX31855 thermocouple(INLET_FLUID_THERMOCOUPLE_CS_PIN);   // Hardware SPI implementation

// PID object for rope heater (https://github.com/Dlloydev/QuickPID/blob/master/examples/PID_RelayOutput/PID_RelayOutput.ino)
QuickPID ropePID(&inlet_fluid_temp_measured, &rope_control_output, &inlet_fluid_temp_setpoint, 
				 rope_Kp, rope_Ki, rope_Kd,
				 QuickPID::pMode::pOnError,
				 QuickPID::dMode::dOnMeas,
				 QuickPID::iAwMode::iAwClamp,
				 QuickPID::Action::direct);

// Create timers
Timer statusLEDTimer;
Timer measureSensorsAndSendDataTimer;
uint16_t measure_count = 0;


// Blink the status LED (callback function for the timer)
void blink_status_led() {
    blink_led_state = !blink_led_state;
    digitalWrite(STATUS_PIN, blink_led_state);
}


// Measure the flow sensors (callback function for the timer)
void measure_sensors() {
	// -------------------------- THERMISTORS --------------------------
	// Measure the thermistors, average over 3 readings
	uint8_t num_average = 3;
	for (int i = 0; i < NUM_THERMISTORS; i++) {
		// Average the thermistor readings
		float thermistor_reading = 0;
		for (int j = 0; j < num_average; j++) {
			thermistor_reading += analogRead(THERMISTOR_PINS[i]);
		}
		thermistor_reading /= num_average;
		// Calculate the temperature for the given thermistor
		thermistor_temps[i] = convert_thermistor_analog_to_celcius(thermistor_reading, max_analog, analog_vref, thermistor_R1);
	}
	
	// -------------------------- INLET FLUID THERMOCOUPLE --------------------------
	// Measure the temperature of the inlet fluid (thermocouple)			
	inlet_fluid_temp_measured = (float)thermocouple.readCelsius();   	// Cast to float from double
}


// -------------------------------- SERIAL IO STUFF -------------------------------- //
// Send data to the PC (callback function for the timer)
void send_data_to_pc(){
	// First measure the sensors
	measure_sensors();

	// Write the struct to send to the PC
	monitor_to_pc_data.time_ms 					= elapsed_millis;
	monitor_to_pc_data.time_us 					= elapsed_micros;
	monitor_to_pc_data.inlet_fluid_temp 		= inlet_fluid_temp_measured;
	monitor_to_pc_data.inlet_fluid_temp_setpoint = inlet_fluid_temp_setpoint;
	monitor_to_pc_data.heater_block_enable 		= heater_block_enable;
	monitor_to_pc_data.rope_heater_enable 		= rope_heater_enable;
	monitor_to_pc_data.heat_flux 				= heat_flux;
	monitor_to_pc_data.thermistor_1_temp_c 		= thermistor_temps[0];
	monitor_to_pc_data.thermistor_2_temp_c 		= thermistor_temps[1];
	monitor_to_pc_data.thermistor_3_temp_c 		= thermistor_temps[2];
	monitor_to_pc_data.thermistor_4_temp_c 		= thermistor_temps[3];
	monitor_to_pc_data.thermistor_5_temp_c 		= thermistor_temps[4];
	monitor_to_pc_data.thermistor_6_temp_c 		= thermistor_temps[5];
	monitor_to_pc_data.thermistor_7_temp_c 		= thermistor_temps[6];
	monitor_to_pc_data.thermistor_8_temp_c 		= thermistor_temps[7];
	monitor_to_pc_data.thermistor_9_temp_c 		= thermistor_temps[8];
	monitor_to_pc_data.thermistor_10_temp_c 	= thermistor_temps[9];
	monitor_to_pc_data.thermistor_11_temp_c 	= thermistor_temps[10];
	monitor_to_pc_data.thermistor_12_temp_c 	= thermistor_temps[11];
	monitor_to_pc_data.thermistor_13_temp_c 	= thermistor_temps[12];
	monitor_to_pc_data.thermistor_14_temp_c 	= thermistor_temps[13];

	// Now send the struct
	ETout_pc.sendData();
}

void save_data_from_pc(){
	// See if time needs to be reset
	if (pc_to_monitor_data.reset_time){
		elapsed_millis = 0;
		elapsed_micros = 0;
		blink_led_state = false;
		// Also reset the blink timer to try and sync with the other MCUs
		statusLEDTimer.reset();
	}

	// Save all other variables to local variables
	inlet_fluid_temp_setpoint 	= pc_to_monitor_data.inlet_fluid_temp_setpoint;
	heater_block_enable 		= pc_to_monitor_data.heater_block_enable;
	rope_heater_enable 			= pc_to_monitor_data.rope_heater_enable;
	heat_flux 					= pc_to_monitor_data.heat_flux;
}

// Used when there is a blocking error, blinks the build-in LED quickly
void fast_led_blink(){
	// Blink the LED at 10 Hz
	digitalWriteFast(STATUS_PIN, HIGH);
	delay(50);
	digitalWriteFast(STATUS_PIN, LOW);
	delay(50);
}

// -------------------------------- SETUP --------------------------------
void setup() {
	// Initialise the EasyTransfer objects
	PC_COMMUNICATION.begin(BAUDRATE);
	ETout_pc.begin(details(monitor_to_pc_data), &PC_COMMUNICATION);
	ETin_pc.begin(details(pc_to_monitor_data), &PC_COMMUNICATION);

	// clear the serial port buffer
	if (PC_COMMUNICATION.available()) {
		PC_COMMUNICATION.read(); 
	}
	delay(100);

	// --------------------------- THERMOCOUPLE SETUP ---------------------------
	thermocouple.begin();							// on SPI bus

	// --------------------------- ANALOG SETTINGS SETUP ---------------------------
	// Set the ADC settings
	analogReadResolution(analog_resolution);		// Set the ADC resolution to n bits (set in config.h)
	analogReference(analog_vref);					// Set the ADC reference voltage to 3.3V rather than 5V, using external power supply set to 3.3V via oscilliscope reading
	analogWriteResolution(10);		// Set the DAC resolution to n bits (set in config.h)
	analogWriteFrequency(HEATER_BLOCK_RELAY_CONTROL_PIN, 100);	// Set the PWM frequency to 100 Hz
	//analogWriteFrequency(ROPE_HEATER_RELAY_CONTROL_PIN, 2000);	// Set the PWM frequency to 20 kHz

	// --------------------------- PIN SETUP ---------------------------	
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
	pinMode(THERMISTOR_13_INPUT_PIN, INPUT);
	pinMode(THERMISTOR_14_INPUT_PIN, INPUT);

	// --------------------------- TIMER OBJECT SETUP ---------------------------
	statusLEDTimer.setInterval(blink_delay_ms, -1);
	statusLEDTimer.setCallback(blink_status_led);
	statusLEDTimer.start();
	measureSensorsAndSendDataTimer.setInterval(measure_and_send_data_delay_ms, -1);
	measureSensorsAndSendDataTimer.setCallback(send_data_to_pc);		// Measure sensors and send data to PC
	measureSensorsAndSendDataTimer.start();

	// --------------------------- INLET FLUID TEMP PID SETUP ---------------------------
	ropePID.SetOutputLimits(0, window_size_ms);
	ropePID.SetSampleTimeUs(window_size_ms * 1000);
	ropePID.SetMode(QuickPID::Control::automatic);
}

void loop() {
	// --------------------------- Update the timer objects ---------------------------
	statusLEDTimer.update();
	measureSensorsAndSendDataTimer.update();

	// --------------------------- CHECK EASY TRANSFER IN DATA ---------------------------
	new_data = false;		// Reset each loop, if something depends on new data to update, then put that later in the loop
	if (ETin_pc.receiveData()){
		save_data_from_pc();
		new_data = true;	// Flag to use for anything that needs to be updated when new data is received
	}

	// --------------------------- Control the rope heater PID loop ---------------------------
	// Run the PID loop, the temperatures setpoint is automatically updated via the save_data_from_pc function
	// rope_control_output is the TIME in milliseconds that the rope heater should be on, this is calculated in the PID controller
	// the control pins need to be PWM enabled for this to work
	if (rope_heater_enable){
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
	} else {
		// Make sure the rope heater is off if it's disabled
		digitalWrite(ROPE_HEATER_RELAY_CONTROL_PIN, LOW);
	}

	// --------------------------- Control the heater block ---------------------------
	// Update the PWM output of the heater block pin to match the desired heat flux
	if (heater_block_enable){
		heat_flux_pwm = map(heat_flux, 0, max_heat_flux, 0, 1023);
		analogWrite(HEATER_BLOCK_RELAY_CONTROL_PIN, heat_flux_pwm);
	} else {
		// Make sure the heater block is off if it's disabled
		analogWrite(HEATER_BLOCK_RELAY_CONTROL_PIN, 0);
	}
	
	delay(2);		// Delay to allow the data logger to run, this is the minimum delay for the data logger to run
}