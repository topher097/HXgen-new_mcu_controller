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
#include <pageManager.h>            // PageManager library, brings Simple_GFX and all other class objest into scope
#include <EasyTransfer.h>			// EasyTransfer library
#include <DataLogger.h>				// DataLogger library (in lib folder)

// Local includes
#include "config.h"
#include "pages/main_page.h"
#include "pages/piezo_page.h"
#include "pages/inlet_temp_page.h"
#include "pins_monitor.h"

#define TO_MEGA_SERIAL Serial1		// Serial port to mega (monitor teensy using rx1 and tx1 pins)
#define TO_DRIVER_SERIAL Serial8	// Serial port to driver (monitor teensy using rx8 and tx8 pins)
String temp;						// Temporary string for printing to serial monitor

// Init the screen and touchscreen objects
//Adafruit_HX8357 tftlcd = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
// T//ouchScreen touch_screen(TOUCH_XP, TOUCH_YP, TOUCH_XM, TOUCH_YM, screen_touch_resistance);

// // Create the Simple_GFX object
// uint16_t tft_width = 480;       // Can set these to the dimensions of your screen
// uint16_t tft_height = 320;     
// uint16_t rotation = 3;
// Simple_GFX<Adafruit_HX8357> sim_gfx(&tftlcd, rotation, tft_width, tft_height);

// // // Init the PageManager object
// PageManager<Adafruit_HX8357> page_manager;

// // Init the Page objects
// MainPage<Adafruit_HX8357> main_page(&page_manager, &sim_gfx, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);
// //PiezoPage piezo_page(&page_manager, &tft, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);
// //InletTempPage inlet_temp_page(&page_manager, &tft, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);

// Initialize the EasyTransfer object
EasyTransfer ETin_driver;			// EasyTransfer object for receiving data from the Driver for flow sensor data
EasyTransfer ETout_driver;			// EasyTransfer object for sending data to the Driver Teensy
EasyTransfer ETin_pc;				// EasyTransfer object for receiving data from the PC
EasyTransfer ETout_pc;				// EasyTransfer object for sending data to the PC

// // Initialize the data logger object
DataLogger data_logger;

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
Timer measureSensorsAndUpdateOutputTimer;
Timer updateScreenTimer;
Timer sendDataToPCTimer;
uint16_t measure_count = 0;


// Blink the status LED (callback function for the timer)
void blink_status_led() {
    blink_led_state = !blink_led_state;
    digitalWrite(STATUS_PIN, blink_led_state);
}


// Measure the flow sensors (callback function for the timer)
void measure_sensors() {
	// -------------------------- INLET FLOW SENSORS FROM THE DRIVER --------------------------
	// Pulse the trigger pin to the driver to measure the flow sensors and send the struct
	digitalWrite(MEGA_MEASURE_FLOW_TRIGGER_PIN, HIGH);
	// Wait for the data from the driver
	while (!ETin_driver.receiveData()) {
		delay(1);
	}
	digitalWrite(MEGA_MEASURE_FLOW_TRIGGER_PIN, LOW);
	// Save the data from the driver
	inlet_flow_sensor_ml_min = driver_to_monitor_data.inlet_flow_sensor_ml_min;
	outlet_flow_sensor_ml_min = driver_to_monitor_data.outlet_flow_sensor_ml_min;

	// -------------------------- THERMISTORS --------------------------
	// Measure the thermistors
	for (int i = 0; i < NUM_THERMISTORS; i++) {
		int reading = analogRead(THERMISTOR_PINS[i]);
		thermistor_temps[i] = convert_thermistor_analog_to_celcius(reading, max_analog, analog_vref, thermistor_R1);
		//thermistor_temps[i] = float(reading);
	}
	
	// -------------------------- INLET FLUID THERMOCOUPLE --------------------------
	// Measure the temperature of the inlet fluid (thermocouple)			
	inlet_fluid_temp_measured = (float)thermocouple.readCelsius(); 
	//inlet_fluid_temp_measured = 123.45;
}

// -------------------------------- DATA LOGGER FUNCTIONS --------------------------------
// https://forum.pjrc.com/threads/60809-Generic-data-logger-object/page4?highlight=fast+datalogger
#define SAMPLERATE 100				// Samples per second to collect and save (ms time between samples is 1/SAMPLERATE)
#define BUFFERMSEC 200				// Buffer limit write speed in milliseconds
#define SYNCINTERVAL 1000			// Sync interval in milliseconds
char logfilename[64];				// filename for the log file
#define MAXBUFFER 8000000			// Make sure that enough room is left for the rest of the program
uint8_t *mybuffer = (uint8_t *)0x80000000;								// pointer to the buffer
const char compileTime [] = "  Compiled on " __DATE__ " " __TIME__;		// Compile time for this program
bool logging = false;				// flag  if logging is running or not
bool endplayback = true;			// flag to end the playback
uint32_t pbrecnum, verifyerrors;	// playback record number and verify errors
TLoggerStat *lsptr;					// pointer to the logger status structure
uint32_t numrecs = 0;				// number of records collected
uint8_t *bufferend;					// pointer to end of buffer
elapsedMillis filemillis;			// elapsed time since file opened in millis
elapsedMicros filemicros;			// elapsed time since file opened in micros


// Function to callback during an interval timer for the data logger (must be global scope)
// This is used to transfer back to the DataLogger object so that the buffering can use all
// the datalogger private variables.
void logger_isr(void){	
	data_logger.TimerChore();		// DataLogger method
}

// Create the filename for the data logger 
void create_file_name(char *filename) {
	sprintf(filename, "HX_%llu.bin", now());
	//Serial.printf("File name is:  <%s>\n", filename);
}

// Function called to start the logging to the SD card
void start_logging(void) {
	// Pause the slower timer for measuring the sensors, the measuring will then be done in the collector
	measureSensorsAndUpdateOutputTimer.pause();
	//Serial.println("Starting Logger.");
	logging = true;
	create_file_name(logfilename);
	numrecs = 0;
	data_logger.StartLogger(logfilename, SYNCINTERVAL, &logger_isr);  // sync once per second
	filemillis = 0; // Opening file takes ~ 40mSec
	filemicros = 0;
	//Serial.print("\n");
}

// Verify the last file by reading it back and comparing to the buffer
void verify_file(void) {
	//Serial.println("Verifying file.");
	endplayback = false;
	pbrecnum = 0;
	verifyerrors = 0;
	data_logger.PlaybackFile(logfilename);
	
	while (!endplayback) {
		delay(2);
	}

	// Serial.printf("Verification errors:  %lu\n", verifyerrors);
	// Serial.println("Verification complete");
}

// Stop the data logger and close file
void quit_logging(void) {
	//Serial.println("Stopping Logger.");
	data_logger.StopLogger();
	logging = false;
	measureSensorsAndUpdateOutputTimer.start();	// Start the slower timer for measuring sensors
}

// Display the status of the logging, can be called before, during, and after logging.
void get_status(void) {
	TLoggerStat *tsp;
	float freembytes, mbytes;
	tsp =  data_logger.GetStatus();
	freembytes = tsp->spaceavailable / (1024.0 * 1024.0);
	mbytes = tsp->byteswritten / (1024.0 * 1024.0);
	// Serial.println("\nLogger Status:");
	// Serial.printf("MBytes Written: %8.2f  ", mbytes);
	// Serial.printf(" SdCard free space: %8.2f MBytes\n", freembytes);
	// Serial.printf("Collection time: %lu seconds\n", tsp->collectionend - tsp->collectionstart);
	// Serial.printf("Max Collection delay: %lu microseconds\n", tsp->maxcdelay);
	// Serial.printf("Average Write Time: %6.3f milliseconds\n", tsp->avgwritetime / 1000.0);
	// Serial.printf("Maximum Write Time: %6.3f milliseconds\n\n", tsp->maxwritetime / 1000.0);
}

// Callback function called from the datalogger time handler
// Write values to the save_data struct to then be sent to the Monitor and be used in the pages
void write_data_save_struct( void* vdp ) {
	// Measure the sensors
	measure_sensors();

	volatile struct dataSave *dp;
	dp = (volatile struct dataSave *)vdp;
	dp->time_stamp_ms = filemillis;
	dp->time_stamp_us = filemicros;
	dp->heat_flux = heat_flux;
	dp->inlet_flow_sensor_ml_min = inlet_flow_sensor_ml_min;
	dp->outlet_flow_sensor_ml_min = outlet_flow_sensor_ml_min;
	dp->inlet_fluid_temp = inlet_fluid_temp_measured;
	dp->inlet_fluid_temp_setpoint = inlet_fluid_temp_setpoint;
	dp->piezo_1_freq = piezo_1_freq;
	dp->piezo_2_freq = piezo_2_freq;
	dp->piezo_1_vpp = piezo_1_vpp;
	dp->piezo_2_vpp = piezo_2_vpp;
	dp->piezo_1_phase = piezo_1_phase;
	dp->piezo_2_phase = piezo_2_phase;
	dp->heater_block_enable = heater_block_enable;
	dp->rope_heater_enable = rope_heater_enable;
	dp->piezo_1_enable = piezo_1_enable;
	dp->piezo_2_enable = piezo_2_enable;
	dp->heat_flux = heat_flux;
	dp->thermistor_1_temp_c = thermistor_temps[0];
	dp->thermistor_2_temp_c = thermistor_temps[1];
	dp->thermistor_3_temp_c = thermistor_temps[2];
	dp->thermistor_4_temp_c = thermistor_temps[3];
	dp->thermistor_5_temp_c = thermistor_temps[4];
	dp->thermistor_6_temp_c = thermistor_temps[5];
	dp->thermistor_7_temp_c = thermistor_temps[6];
	dp->thermistor_8_temp_c = thermistor_temps[7];
	dp->thermistor_9_temp_c = thermistor_temps[8];
	dp->thermistor_10_temp_c = thermistor_temps[9];
	dp->thermistor_11_temp_c = thermistor_temps[10];
	dp->thermistor_12_temp_c = thermistor_temps[11];
	dp->thermistor_13_temp_c = thermistor_temps[12];

	numrecs++;								// Increment the number of records

	// // If first recordings, then print to serial monitor as format microtime, millitime, numrecords, cptr
	// if (numrecs < 20){
	//   Serial.printf("%lu, %lu, %lu, %p\n", dp->microtime, dp->millitime, dp->numrecords, dp->cptr);
	// }
	// else {
	//   quit_logging();
	// }
};


// -------------------------------- SCREEN UPDATE VIA PAGE MANAGER --------------------------------
// Update the screen with relavent information (callback function for the timer)
void update_screen() {
	// Update the screen
	//page_manager.update_current_page();
}

// -------------------------------- SERIAL INPUT STUFF --------------------------------
uint8_t incoming_data_buffer[sizeof(dataSave) + 2];		// buffer for incoming serial data, including the start flag
uint8_t incoming_data_index = 0;
struct dataSave monitor_to_pc_data;						// struct to hold the outgoing data to PC
uint8_t *dummy_cptr;

void save_data_from_driver(){
	// Save the global variables via the driver_to_monitor_data struct
	inlet_flow_sensor_ml_min = driver_to_monitor_data.inlet_flow_sensor_ml_min;
	outlet_flow_sensor_ml_min = driver_to_monitor_data.outlet_flow_sensor_ml_min;
}

void send_data_to_driver(){
	// Write global data to the monitor_to_driver_data struct
	monitor_to_driver_data.piezo_1_enable = piezo_1_enable;
	monitor_to_driver_data.piezo_2_enable = piezo_2_enable;
	monitor_to_driver_data.piezo_1_freq = piezo_1_freq;
	monitor_to_driver_data.piezo_2_freq = piezo_2_freq;
	monitor_to_driver_data.piezo_1_phase = piezo_1_phase;
	monitor_to_driver_data.piezo_2_phase = piezo_2_phase;
	monitor_to_driver_data.piezo_1_vpp = piezo_1_vpp;
	monitor_to_driver_data.piezo_2_vpp = piezo_2_vpp;

	ETout_driver.sendData();
	new_piezo_data = false;		// Reset the flag
}
	
void send_data_to_pc(){
	// Write the struct
	monitor_to_pc_data.time_stamp_us = filemicros;
	monitor_to_pc_data.time_stamp_ms = filemillis;
	monitor_to_pc_data.heat_flux = heat_flux;
	monitor_to_pc_data.inlet_flow_sensor_ml_min = inlet_flow_sensor_ml_min;
	monitor_to_pc_data.outlet_flow_sensor_ml_min = outlet_flow_sensor_ml_min;
	monitor_to_pc_data.inlet_fluid_temp = inlet_fluid_temp_measured;
	monitor_to_pc_data.inlet_fluid_temp_setpoint = inlet_fluid_temp_setpoint;
	monitor_to_pc_data.piezo_1_freq = piezo_1_freq;
	monitor_to_pc_data.piezo_2_freq = piezo_2_freq;
	monitor_to_pc_data.piezo_1_vpp = piezo_1_vpp;
	monitor_to_pc_data.piezo_2_vpp = piezo_2_vpp;
	monitor_to_pc_data.piezo_1_phase = piezo_1_phase;
	monitor_to_pc_data.piezo_2_phase = piezo_2_phase;
	monitor_to_pc_data.heater_block_enable = heater_block_enable;
	monitor_to_pc_data.rope_heater_enable = rope_heater_enable;
	monitor_to_pc_data.piezo_1_enable = piezo_1_enable;
	monitor_to_pc_data.piezo_2_enable = piezo_2_enable;
	monitor_to_pc_data.heat_flux = heat_flux;
	monitor_to_pc_data.thermistor_1_temp_c = thermistor_temps[0];
	monitor_to_pc_data.thermistor_2_temp_c = thermistor_temps[1];
	monitor_to_pc_data.thermistor_3_temp_c = thermistor_temps[2];
	monitor_to_pc_data.thermistor_4_temp_c = thermistor_temps[3];
	monitor_to_pc_data.thermistor_5_temp_c = thermistor_temps[4];
	monitor_to_pc_data.thermistor_6_temp_c = thermistor_temps[5];
	monitor_to_pc_data.thermistor_7_temp_c = thermistor_temps[6];
	monitor_to_pc_data.thermistor_8_temp_c = thermistor_temps[7];
	monitor_to_pc_data.thermistor_9_temp_c = thermistor_temps[8];
	monitor_to_pc_data.thermistor_10_temp_c = thermistor_temps[9];
	monitor_to_pc_data.thermistor_11_temp_c = thermistor_temps[10];
	monitor_to_pc_data.thermistor_12_temp_c = thermistor_temps[11];
	monitor_to_pc_data.thermistor_13_temp_c = thermistor_temps[12];
	monitor_to_pc_data.thermistor_14_temp_c = thermistor_temps[13];
	monitor_to_pc_data.numrecords = numrecs;
	monitor_to_pc_data.cptr = dummy_cptr;

	// Now send the struct
	ETout_pc.sendData();
}

void save_data_from_pc(){
	// Save the global variables via the pc_to_monitor_data struct
	inlet_fluid_temp_setpoint = pc_to_monitor_data.inlet_fluid_temp_setpoint;
	heater_block_enable = pc_to_monitor_data.heater_block_enable;
	rope_heater_enable = pc_to_monitor_data.rope_heater_enable;
	piezo_1_enable = pc_to_monitor_data.piezo_1_enable;
	piezo_2_enable = pc_to_monitor_data.piezo_2_enable;
	piezo_1_freq = pc_to_monitor_data.piezo_1_freq;
	piezo_2_freq = pc_to_monitor_data.piezo_2_freq;
	piezo_1_vpp = pc_to_monitor_data.piezo_1_vpp;
	piezo_2_vpp = pc_to_monitor_data.piezo_2_vpp;
	piezo_1_phase = pc_to_monitor_data.piezo_1_phase;
	heat_flux = pc_to_monitor_data.heat_flux;

	// Set the flags
	new_piezo_data = true;
}

void fast_led_blink(){
	// Blink the LED at 10 Hz
	digitalWriteFast(LED_BUILTIN, HIGH);
	delay(50);
	digitalWriteFast(LED_BUILTIN, LOW);
	delay(50);
}

// -------------------------------- SETUP --------------------------------
void setup() {
	// Initialise the EasyTransfer objects
	//ETin_mega.begin(details(mega_data), &TO_MEGA_SERIAL);
	ETout_driver.begin(details(monitor_to_driver_data), &Serial8);
	ETin_driver.begin(details(driver_to_monitor_data), &Serial8);
	ETout_pc.begin(details(monitor_to_pc_data), &Serial);
	ETin_pc.begin(details(pc_to_monitor_data), &Serial);

	// Initialize the serial communication
    Serial.begin(BAUDRATE);
	Serial8.begin(BAUDRATE);
	//if (wait_for_serial) {while (!Serial) {};};			// wait for serial port to connect. Needed for native USB port only
	
	if (Serial.available()) {
		Serial.read(); // clear the serial port
	}

	delay(100);
	//Serial.println(F("Starting..."));

	// --------------------------- DATA LOGGER SETUP ---------------------------
	uint32_t bufflen;
	data_logger.SetDBPrint(true);  		// turn on debug output	
	// try starting SD Card and file system
	if (!data_logger.InitStorage(NULL)) { 	
		// IF INITIALIZATION FAILS, LOOP FOREVER while fast flashing LED
		while (1) {fast_led_blink();}
	}
	// now try to initialize buffer.  Check that request falls within
	// size of local buffer or if it will fit on heap, save_data is in data_transfer.h
	bufflen = data_logger.InitializeBuffer(sizeof(dataSave), SAMPLERATE, BUFFERMSEC, mybuffer);
	bufferend = mybuffer + bufflen;
	//Serial.printf("End of buffer at %p\n", bufferend);
	if ((bufflen == 0) || (bufflen > MAXBUFFER)) {
		// If not successful, loop forever while fast flashing LED. Reduce SAMPLERATE or BUFFERMSEC
		while (1) {fast_led_blink();}
	}
	data_logger.AttachCollector(&write_data_save_struct);		// attach the customized callback function
	//data_logger.AttachDisplay(&my_binary_display, 5000);		// display written data once per 5 seconds
	//data_logger.AttachPlayback(&my_verify);						// check for missing records

	// --------------------------- THERMOCOUPLE SETUP ---------------------------
	thermocouple.begin();						// on SPI bus
	
	// --------------------------- SIMPLE_GFX TFT DISPLAY SETUP ---------------------------
	//sim_gfx.init();
	//sim_gfx.fillScreen(HX8357_GREEN);

	// --------------------------- TFT DISPLAY SETUP CHECKING ---------------------------
    // Check the Simple_GFX pointers are correct and compare the address of the tftlcs and TCTClass
    // if (sim_gfx.check_tft_pointer_exists()){
    //     Serial.print(F("The address of tftlcd in main.cpp is: ")); Serial.println((uintptr_t)&tftlcd, HEX);
    //     Serial.print(F("The address of TFTClass in sim_gfx is: ")); Serial.println(sim_gfx.get_tft_address_as_int(), HEX);
    // } else {
    //     Serial.println(F("The pointer to TFTClass is invalid in Simple_GFX object"));
    // }

    // // Print the address to sim_gfx object
    // Serial.print(F("The address of sim_gfx in main.cpp is: ")); Serial.println((uintptr_t)&sim_gfx, HEX);
    // //main_page.get_sim_gfx_pointer_addr();

	// --------------------------- ANALOG SETTINGS SETUP ---------------------------
	// Set the ADC settings
	analogReadResolution(analog_resolution);		// Set the ADC resolution to n bits (set in config.h)
	analogReference(analog_vref);					// Set the ADC reference voltage to 3.3V rather than 5V

	// --------------------------- PIN SETUP ---------------------------	
	// Setup the pins for the Teensy input and output
	pinMode(HEATER_BLOCK_RELAY_CONTROL_PIN, OUTPUT);
	pinMode(ROPE_HEATER_RELAY_CONTROL_PIN, OUTPUT);
	pinMode(STATUS_PIN, OUTPUT);
	pinMode(MEGA_MEASURE_FLOW_TRIGGER_PIN, OUTPUT);
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
	measureSensorsAndUpdateOutputTimer.setInterval(measurement_update_delay_ms, -1);
	measureSensorsAndUpdateOutputTimer.setCallback(measure_sensors);
	measureSensorsAndUpdateOutputTimer.start();
	// updateScreenTimer.setInterval(screen_update_delay_ms, -1);
	// updateScreenTimer.setCallback(update_screen);
	// updateScreenTimer.start();
	sendDataToPCTimer.setInterval(send_data_delay_ms, -1);
	sendDataToPCTimer.setCallback(send_data_to_pc);
	sendDataToPCTimer.start();

	// --------------------------- PID SETUP ---------------------------
	ropePID.SetOutputLimits(0, window_size_ms);
	ropePID.SetSampleTimeUs(window_size_ms * 1000);
	ropePID.SetMode(QuickPID::Control::automatic);
	//Serial.println(F("Finished setting up the PID controller"));


	// // --------------------------- PAGE MANAGER SETUP ---------------------------
	// page_manager.add_page(MAIN_PAGE, &main_page);
	// //Serial.println("Added the main page to manager");
	// //page_manager.add_page(INLET_TEMP_PAGE, &inlet_temp_page);
	// //Serial.println("Added the inlet temp page to manager");
	// //page_manager.add_page(HEATER_FLUX_PAGE, &heater_flux_page);
	// //page_manager.add_page(PIEZO_CONFIG_PAGE, &piezo_page);
	// page_manager.set_page(MAIN_PAGE);		// Set the first page to main page
	// //Serial.println("Set the main page as the current page");
	//Serial.println(F("Finished setup"));
}

void loop() {
	// --------------------------- Update the timer objects ---------------------------
	statusLEDTimer.update();
	measureSensorsAndUpdateOutputTimer.update();
	//updateScreenTimer.update();
	sendDataToPCTimer.update();

	// --------------------------- CHECK EASY TRANSFER IN DATA ---------------------------
	// Check if data is available to read
	// if (Serial8.available()){
	// 	ETin_driver.receiveData();		// Read the data from the serial port
	// 	save_data_from_driver();
	// }
	if (ETin_pc.receiveData()){
		save_data_from_pc();
	}

	// --------------------------- Update the EasyTransfer OUT ---------------------------
	if (new_piezo_data){
		send_data_to_driver();
	}

	// --------------------------- Update the data logger ---------------------------
	TLoggerStat *tsp;
	tsp =  data_logger.GetStatus();	
	data_logger.CheckLogger();     		// Check for data to write to SD at regular intervals
	if (logging) {
		if (tsp->spaceavailable < (10240l * 1024)) { //we are within 10MB of end of card
			//Serial.println(F("SD card is nearly full! Halting logging!"));
			quit_logging();
			get_status();
			fast_led_blink();
		}
	}
	
	// Check for the start/stop recording flag from the Monitor (run once when the flag is set from TFT button)
	if (start_stop_recording && !logging) {
		// Start the data logger, logging flag is set in start_logging()
		//Serial.println("Start logging.");
		start_logging();
	}
	if (!start_stop_recording && logging) {
		// Stop the data logger, logging flag is set in quit_logging()
		//Serial.println("Stop logging.");
		quit_logging();
	} 
		
	// --------------------------- Control the rope heater PID loop ---------------------------
	// Run the PID loop
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
	}

	// --------------------------- Control the heater block ---------------------------
	// Update the PWM output of the heater block pin to match the desired heat flux
	if (heater_block_enable){
		heat_flux_pwm = map(heat_flux, 0, max_heat_flux, 0, 255);
		analogWrite(HEATER_BLOCK_RELAY_CONTROL_PIN, heat_flux_pwm);
	}
	
	delay(2);		// Delay to allow the data logger to run, this is the minimum delay for the data logger to run
}