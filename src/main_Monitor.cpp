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
//#include <SoftwareWire.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>				// I2C device library for Teensy 4.1

#include <Adafruit_HX8357.h>		// TFT display library
#include <Adafruit_GFX.h>			// GFX library
#include <Adafruit_MAX31855.h>		// Thermocouple library
#include <timer.h>					// Timer library
#include <QuickPID.h>				// PID library
#include <pageManager.h>            // PageManager library, brings Simple_GFX and all other class objest into scope
#include <EasyTransfer.h>			// EasyTransfer library
#include <Datalogger.h>				// DataLogger library (in lib folder)

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
Simple_GFX<Adafruit_HX8357> sim_gfx(&tftlcd, 1, tft_width, tft_height);

// // Init the PageManager object
PageManager<Adafruit_HX8357> page_manager;

// Init the Page objects
MainPage<Adafruit_HX8357> main_page(&page_manager, &sim_gfx, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);
//PiezoPage piezo_page(&page_manager, &tft, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);
//InletTempPage inlet_temp_page(&page_manager, &tft, &touch_screen, min_pressure, max_pressure, touchXMin, touchXMax, touchYMin, touchYMax);

// Initialize the EasyTransfer object
EasyTransfer ETin;		// EasyTransfer object for receiving data from the Driver Teensy
EasyTransfer ETout;		// EasyTransfer object for sending data to the Driver Teensy

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

// Create FlowSensor objects for the inlet and outlet sensors
const String inlet_string = "inlet";
const String outlet_string = "outlet";
// SoftwareWire inlet_wire(INLET_FLOW_SENSOR_SDA, INLET_FLOW_SENSOR_SCL);
// SoftwareWire outlet_wire(OUTLET_FLOW_SENSOR_SDA, OUTLET_FLOW_SENSOR_SCL);
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
    blink_led_state = !blink_led_state;
    digitalWrite(STATUS_PIN, blink_led_state);
}


// Measure the flow sensors (callback function for the timer)
void measure_sensors() {
	if (print_to_serial) {
		Serial.print(F("Measure count: ")); Serial.print(measure_count); Serial.print(F(",     "));
		measure_count++;
	}
	
	// --------------------------------- FLOW SENSORS ---------------------------------
	inlet_sensor.measure_flow();
	outlet_sensor.measure_flow();
	inlet_flow_sensor_ml_min = inlet_sensor.scaled_flow_value;
	outlet_flow_sensor_ml_min = outlet_sensor.scaled_flow_value;

	// Print the flow rate and temp to the serial monitor
	if (print_to_serial) {
		temp = inlet_sensor.get_name() + ": " + String(inlet_sensor.scaled_flow_value) + " " + String(UNIT_FLOW);
		Serial.print(temp); Serial.print(",     ");
		temp = outlet_sensor.get_name() + ": " + String(outlet_sensor.scaled_flow_value) + " " + String(UNIT_FLOW);
		Serial.print(temp); Serial.print(",     ");
	}

    // --------------------------------- HEATER BLOCK THERMISTORS ---------------------------------
	// Read all of the thermistors


}

// -------------------------------- DATA LOGGER FUNCTIONS --------------------------------
// https://forum.pjrc.com/threads/60809-Generic-data-logger-object/page4?highlight=fast+datalogger
#define SAMPLERATE 50				// Samples per second to collect and save (ms time between samples is 1/SAMPLERATE)
#define BUFFERMSEC 200				// Buffer limit write speed in milliseconds
#define SYNCINTERVAL 1000			// Sync interval in milliseconds
char logfilename[64];

// Make sure that enough room is left for the rest of the program
#define MAXBUFFER 7000000
uint8_t *mybuffer = (uint8_t *)0x40000000;
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
	Serial.printf("File name is:  <%s>\n", filename);
}

// Function called to start the logging to the SD card
void start_logging(void) {
	// Pause the slower timer for measuring the sensors, the measuring will then be done in the collector
	measureSensorsAndUpdateOutputTimer.pause();
	Serial.println("Starting Logger.");
	logging = true;
	create_file_name(logfilename);
	numrecs = 0;
	data_logger.StartLogger(logfilename, 1000, &logger_isr);  // sync once per second
	filemillis = 0; // Opening file takes ~ 40mSec
	filemicros = 0;
	Serial.print("\n");
}

// Verify the last file by reading it back and comparing to the buffer
void verify_file(void) {
	Serial.println("Verifying file.");
	endplayback = false;
	pbrecnum = 0;
	verifyerrors = 0;
	data_logger.PlaybackFile(logfilename);
	
	while (!endplayback) {
		delay(2);
	}

	Serial.printf("Verification errors:  %lu\n", verifyerrors);
	Serial.println("Verification complete");
}

// Stop the data logger and close file
void quit_logging(void) {
	Serial.println("Stopping Logger.");
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
	Serial.println("\nLogger Status:");
	Serial.printf("MBytes Written: %8.2f  ", mbytes);
	Serial.printf(" SdCard free space: %8.2f MBytes\n", freembytes);
	Serial.printf("Collection time: %lu seconds\n", tsp->collectionend - tsp->collectionstart);
	Serial.printf("Max Collection delay: %lu microseconds\n", tsp->maxcdelay);
	Serial.printf("Average Write Time: %6.3f milliseconds\n", tsp->avgwritetime / 1000.0);
	Serial.printf("Maximum Write Time: %6.3f milliseconds\n\n", tsp->maxwritetime / 1000.0);

}

// Callback function called from the datalogger time handler
// Write values to the save_data struct to then be sent to the Monitor and be used in the pages
void write_data_save_struct( void* vdp ) {
	// Measure the sensors
	measure_sensors();

	volatile struct dataSave *dp;
	dp = (volatile struct dataSave *)vdp;

	// Note: logger status keeps track of max collection time
	dp->time_stamp_ms = filemillis;
	dp->time_stamp_us = filemicros;
	dp->heat_flux = heat_flux;
	dp->inlet_flow_sensor_ml_min = inlet_flow_sensor_ml_min;
	dp->outlet_flow_sensor_ml_min = outlet_flow_sensor_ml_min;
	dp->inlet_fluid_temp = inlet_fluid_temp_measured;
	dp->inlet_fluid_temp_setpoint = inlet_fluid_temp_setpoint;
	dp->rope_heater_enable = rope_heater_enable;
	dp->heater_block_enable = heater_block_enable;
	dp->piezo_1_freq = piezo_1_freq;
	dp->piezo_2_freq = piezo_2_freq;
	dp->piezo_1_vpp = piezo_1_vpp;
	dp->piezo_2_vpp = piezo_2_vpp;
	dp->piezo_1_phase = piezo_1_phase;
	dp->piezo_2_phase = piezo_2_phase;
	dp->piezo_1_enable = piezo_1_enable;
	dp->piezo_2_enable = piezo_2_enable;
	dp->numrecords = numrecs;				
	dp->cptr = (uint8_t *)dp;				// Save the address of the struct, saved as a uint32_t variable (need to know for converting to CSV)

	numrecs++;								// Increment the number of records

	// Save the last data struct for updating the screen
	last_save_data = (volatile struct dataSave *)dp;

	// // If first recordings, then print to serial monitor as format microtime, millitime, numrecords, cptr
	// if (numrecs < 20){
	//   Serial.printf("%lu, %lu, %lu, %p\n", dp->microtime, dp->millitime, dp->numrecords, dp->cptr);
	// }
	// else {
	//   quit_logging();
	// }
};

// Called from the playback. This is used to verify the data
void my_verify(void *vdp) {
	struct dataSave *dp;
	dp = (struct dataSave *)vdp;
	// Check to see if the file playback is ended
	if (dp == NULL) {
		Serial.println("End of file playback.");
		endplayback = true;
		return;
	}
	// Check to see if the data is correct (number of records recorded is the same number as verified)
	if (dp->numrecords != pbrecnum) {
		Serial.printf("Record number error at %lu", dp->numrecords);
		Serial.printf("  File: %lu  local: %lu\n", dp->numrecords, pbrecnum);
		pbrecnum = dp->numrecords;
		verifyerrors++;
	}

	// Increment the verify record number and display the record number every 10000 records
	pbrecnum++;
	if ((pbrecnum % 10000) == 0)Serial.printf("Recnum: %lu\n", pbrecnum);
};

// Called from the datalogger CheckLogger function, displays the dinary point every x amount of seconds
void my_binary_display( void* vdp ) {
	struct dataSave *dp;
	dp = (struct dataSave *)vdp;
	TLoggerStat *tsp;
	tsp =  data_logger.GetStatus(); // updates values collected at interrupt time

	if (!logging) return;
	Serial.printf("%8.3f,  ", dp->time_stamp_ms / 1000.0);
	Serial.printf(" Records:%10lu  ", dp->numrecords);
	Serial.printf(" Overflows: %4lu\n", tsp->bufferoverflows);
	if ( dp->cptr > bufferend) {
		Serial.printf("Saved data outside buffer at %p\n", dp->cptr);
	}
}


// -------------------------------- SCREEN UPDATE VIA PAGE MANAGER --------------------------------
// Update the screen with relavent information (callback function for the timer)
void update_screen() {
	// Update the screen
	page_manager.update_current_page();
}


// -------------------------------- SETUP --------------------------------
void setup() {
	// Initialize the serial communication
    Serial.begin(SERIAL_BAUD);
	if (wait_for_serial) {while (!Serial) {};};			// wait for serial port to connect. Needed for native USB port only
	
	if (Serial.available()) {
		Serial.read(); // clear the serial port
	}

	delay(1500);
	Serial.println(F("Starting..."));
	delay(1500);


	// --------------------------- DATA LOGGER SETUP ---------------------------
	uint32_t bufflen;
	data_logger.SetDBPrint(true);  		// turn on debug output	
	// try starting SD Card and file system
	if (!data_logger.InitStorage(NULL)) { 	
		Serial.println(F("SD card failed to initialize, or file system not found"));
		while (1) {delay(1000);}
	}
	// now try to initialize buffer.  Check that request falls within
	// size of local buffer or if it will fit on heap, save_data is in data_transfer.h
	bufflen = data_logger.InitializeBuffer(sizeof(dataSave), SAMPLERATE, BUFFERMSEC, mybuffer);
	bufferend = mybuffer + bufflen;
	Serial.printf("End of buffer at %p\n", bufferend);
	if ((bufflen == 0) || (bufflen > MAXBUFFER)) {
		Serial.println(F("Not enough buffer space!  Reduce buffer time or sample rate."));
		while (1) {delay(1000);}
	}
	data_logger.AttachCollector(&write_data_save_struct);		// attach the customized callback function
	data_logger.AttachDisplay(&my_binary_display, 5000);		// display written data once per 5 seconds
	//data_logger.AttachWriter(&my_binary_writer);				// attach the customized callback function
	data_logger.AttachPlayback(&my_verify);						// check for missing records

	// --------------------------- FLOW SENSOR AND THERMOCOUPLE SETUP ---------------------------
	thermocouple.begin();						// on SPI bus
	inlet_sensor.begin();
	outlet_sensor.begin();
	inlet_sensor.soft_reset();		
	outlet_sensor.soft_reset();
	inlet_sensor.set_continuous_mode();
	outlet_sensor.set_continuous_mode();
	
	
	// --------------------------- SIMPLE_GFX TFT DISPLAY SETUP ---------------------------
	sim_gfx.init();
	//tftlcd.begin();		// For the display
	sim_gfx.setRotation(1);
	sim_gfx.fillScreen(HX8357_GREEN);

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
	analogReadResolution(analog_resolution);		// Set the ADC resolution to 12 bits (set in config.h)
	analogReference(analog_vref);					// Set the ADC reference voltage to 3.3V rather than 5V

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

	// --------------------------- TIMER OBJECT SETUP ---------------------------
	statusLEDTimer.setInterval(blink_delay_ms, -1);
	statusLEDTimer.setCallback(blink_status_led);
	statusLEDTimer.start();
	measureSensorsAndUpdateOutputTimer.setInterval(measurement_update_delay_ms, -1);
	measureSensorsAndUpdateOutputTimer.setCallback(measure_sensors);
	measureSensorsAndUpdateOutputTimer.start();
	updateScreenTimer.setInterval(screen_update_delay_ms, -1);
	updateScreenTimer.setCallback(update_screen);
	updateScreenTimer.start();

	// --------------------------- PID SETUP ---------------------------
	ropePID.SetOutputLimits(0, window_size_ms);
	ropePID.SetSampleTimeUs(window_size_ms * 1000);
	ropePID.SetMode(QuickPID::Control::automatic);
	//Serial.println(F("Finished setting up the PID controller"));


	// --------------------------- PAGE MANAGER SETUP ---------------------------
	page_manager.add_page(MAIN_PAGE, &main_page);
	//Serial.println("Added the main page to manager");
	//page_manager.add_page(INLET_TEMP_PAGE, &inlet_temp_page);
	//Serial.println("Added the inlet temp page to manager");
	//page_manager.add_page(HEATER_FLUX_PAGE, &heater_flux_page);
	//page_manager.add_page(PIEZO_CONFIG_PAGE, &piezo_page);
	page_manager.set_page(MAIN_PAGE);		// Set the first page to main page
	//Serial.println("Set the main page as the current page");
}

void loop() {
	// --------------------------- Update the timer objects ---------------------------
	statusLEDTimer.update();
	measureSensorsAndUpdateOutputTimer.update();
	//updateScreenTimer.update();


	// --------------------------- Update the EasyTransfer objects ---------------------------



	// --------------------------- Update the data logger ---------------------------
	TLoggerStat *tsp;
	float mbytes;
	tsp =  data_logger.GetStatus();	
	data_logger.CheckLogger();     		// Check for data to write to SD at regular intervals
	if (logging) {
		if (tsp->spaceavailable < (10240l * 1024)) { //we are within 10MB of end of card
			Serial.println(F("SD card is nearly full! Halting logging!"));
			quit_logging();
			get_status();
		}
	}
	
	// Check for the start/stop recording flag from the Monitor (run once when the flag is set from TFT button)
	if (start_stop_recording && !logging) {
		// Start the data logger, logging flag is set in start_logging()
		Serial.println("Start logging.");
		start_logging();
	}
	if (!start_stop_recording && logging) {
		// Stop the data logger, logging flag is set in quit_logging()
		Serial.println("Stop logging.");
		quit_logging();
	} 
		
	// --------------------------- Control the rope heater PID loop ---------------------------
	// // Measure the temperature of the inlet fluid (thermocouple)			
	// inlet_fluid_temp_measured = thermocouple.readCelsius(); 

	// // Run the PID loop
	// // rope_control_output is the TIME in milliseconds that the rope heater should be on, this is calculated in the PID controller
	// // the control pins need to be PWM enabled for this to work
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
	// analogWrite(HEATER_BLOCK_RELAY_CONTROL_PIN, heat_flux_pwm);


	delay(2);		// Delay to allow the data logger to run, this is the minimum delay for the data logger to run
}