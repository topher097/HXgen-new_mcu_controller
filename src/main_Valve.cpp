#include <Arduino.h>
#include <Wire.h>
#include <EasyTransfer.h>
#include <Timer.h>

// Custom includes
#include <config.h>
#include <pins_valve.h>
#include <flow_valve.cpp>

// Create the time objects
elapsedMillis elapsed_millis;			// elapsed time since start recording in milliseconds
elapsedMicros elapsed_micros;			// elapsed time since start recording in micro seconds

// Timers
Timer statusLEDTimer;
Timer measureSensorsAndSendDataTimer;

bool new_data = false;                  // Flag for new data from the PC

// Flow valve object
uint16_t analog_res = 10;
uint16_t max_analog_value = pow(2, analog_res) - 1;
float vref = 3.3;
FlowValve valve(vref, max_analog_value);

// Easy transfer objects
EasyTransfer ETout_pc, ETin_pc;
#define PC_COMMUNICATION Serial     // Use the COM port on the Teensy 4.0

// Blink the status LED (callback function for the timer)
void blink_status_led() {
    blink_led_state = !blink_led_state;
    digitalWrite(STATUS_PIN, blink_led_state);
}

// Used when there is a blocking error, blinks the build-in LED quickly
void fast_led_blink(){
	// Blink the LED at 10 Hz
	digitalWriteFast(STATUS_PIN, HIGH);
	delay(50);
	digitalWriteFast(STATUS_PIN, LOW);
	delay(50);
}

// Measure the sensors
void measure_sensors(){
    // Measure the pressure sensors
    valve.measure();
}

// Save the data just received from the PC
void save_data_from_pc(){
    // See if time needs to be reset
    if (pc_to_valve_data.reset_time){
        elapsed_millis = 0;
        elapsed_micros = 0;
        blink_led_state = false;
        // Reset the blink timer
        statusLEDTimer.reset();
    }
}

void send_data_to_pc(){
    Serial.print("Valve rotation: "); Serial.print(analogRead(POTENTIOMETER_PIN)); Serial.print("\t");

    // First measure the sensors
    measure_sensors();

    Serial.print("Inlet flow: "); Serial.print(valve.inlet_flow_rate); Serial.println();

    // Update the struct with current values
    valve_to_pc_data.time_ms = elapsed_millis;
    valve_to_pc_data.time_us = elapsed_micros;
    valve_to_pc_data.valve_flow_rate_ml_min = valve.inlet_flow_rate;

    

    // Send the data
    //ETout_pc.sendData();
};

// Setup everything
void setup(){
    // Begin the serial port (for PC)
    PC_COMMUNICATION.begin(BAUD_RATE);
    ETin_pc.begin(details(pc_to_valve_data), &PC_COMMUNICATION);
    ETout_pc.begin(details(valve_to_pc_data), &PC_COMMUNICATION);

    // Set the LED pin as an output
    pinMode(STATUS_PIN, OUTPUT);
    // pinMode(POTENTIOMETER_PIN, INPUT);
    // pinMode(UPSTREAM_NEG_PIN, INPUT);
    // pinMode(UPSTREAM_POS_PIN, INPUT);
    // pinMode(DOWNSTREAM_NEG_PIN, INPUT);
    // pinMode(DOWNSTREAM_POS_PIN, INPUT);

    // Set the analog read, overwrite the analog resolution and the max analog value
    analogReadResolution(analog_res);
    analogReadAveraging(32);


    // Set the send data timer
    statusLEDTimer.setInterval(blink_delay_ms, -1);
    statusLEDTimer.setCallback(blink_status_led);
    statusLEDTimer.start();
    measureSensorsAndSendDataTimer.setInterval(measure_and_send_data_delay_ms, -1);
	measureSensorsAndSendDataTimer.setCallback(send_data_to_pc);		// Measure sensors and send data to PC
	measureSensorsAndSendDataTimer.start();
};


void loop(){
    //fast_led_blink();

    // --------------------------- Update the timer objects ---------------------------
    statusLEDTimer.update();
    measureSensorsAndSendDataTimer.update();

    // --------------------------- CHECK EASY TRANSFER IN DATA ---------------------------
	new_data = false;		// Reset each loop, if something depends on new data to update, then put that later in the loop
	if (ETin_pc.receiveData()){
		save_data_from_pc();
		new_data = true;	// Flag to use for anything that needs to be updated when new data is received
	}
};