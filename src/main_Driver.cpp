/*
Main file for the Teensy 4.0 that:
    communicates with the Teensy Monitor (4.1) to generate the signal for the piezo amplifier via EasyTransfer,
*/
//#include <i2c_driver_wire.h>

#include <Arduino.h>
#include <Wire.h>
#include <Audio.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
// #include <SoftwareWire.h>
#include <EasyTransfer.h>
#include <timer.h>

// Custom includes
#include <config.h>
//#include <data_transfer.h>
#include <flow_sensor.cpp>
#include <pins_driver.h>

// Create the time objects
elapsedMillis elapsed_millis;			// elapsed time since start recording in milliseconds
elapsedMicros elapsed_micros;			// elapsed time since start recording in micro seconds

bool new_data = false;                    // flag to indicate new data is available from the PC

// Easy transfer objects
#define PC_COMMUNICATION Serial
EasyTransfer ETout_pc, ETin_pc;

// Flow sensor objects
const String inlet_string = "inlet";
const String outlet_string = "outlet";
FlowSensor inlet_sensor(inlet_string, &Wire);
FlowSensor outlet_sensor(outlet_string, &Wire1);

// Timers
Timer statusLEDTimer;
Timer measureSensorsAndSendDataTimer;

// Calculated amplitude which is used when setting the amplitude for waveforms
float piezo_1_amplitude = 0.0;
float piezo_2_amplitude = 0.0;

// Create different waveforms for the signal to the piezo amplifier
// Generic wave form generator (sine, square, triangle, sawtooth, etc.)
AudioSynthWaveform       piezo_1_waveform;  
AudioSynthWaveform       piezo_2_waveform; 

// Generic random noise generator
AudioSynthNoiseWhite     piezo_1_noise;   
AudioSynthNoiseWhite     piezo_2_noise;    

// Tone generator with sweep (sine wave)
AudioSynthToneSweep      piezo_1_tonesweep;     
AudioSynthToneSweep      piezo_2_tonesweep;     

// Create outputs for the piezos sent to the amplifier
AudioOutputI2S           i2s1;           // Using the left and right output pins

// Mixer for the two piezo signals
AudioMixer4              piezo_1_mixer;
AudioMixer4              piezo_2_mixer;

// Create all of the connections necessary for the audio objects
// Set the channels for the mixer and output
int piezo_1_channel = 0;        // Left channel  (yellow wire)
int piezo_2_channel = 1;        // Right channel (red wire)
int synthwave_channel = 0;
int noise_channel = 1;
int sweep_channel = 2;

// Create connections for the signals
AudioConnection          piezo_1_synthwave_connection(piezo_1_waveform, 0, piezo_1_mixer, synthwave_channel);
AudioConnection          piezo_2_synthwave_connection(piezo_2_waveform, 0, piezo_2_mixer, synthwave_channel);
AudioConnection          piezo_1_noise_connection(piezo_1_noise, 0, piezo_1_mixer, noise_channel);
AudioConnection          piezo_2_noise_connection(piezo_2_noise, 0, piezo_2_mixer, noise_channel);
AudioConnection          piezo_1_tonesweep_connection(piezo_1_tonesweep, 0, piezo_1_mixer, sweep_channel);
AudioConnection          piezo_2_tonesweep_connection(piezo_2_tonesweep, 0, piezo_2_mixer, sweep_channel);

// Setup the output from mixers to the output pins
AudioConnection          piezo_1_output_signal(piezo_1_mixer, 0, i2s1, piezo_1_channel);
AudioConnection          piezo_2_output_signal(piezo_2_mixer, 0, i2s1, piezo_2_channel);

AudioControlSGTL5000     sgtl5000_1;     // Create the SGTL5000 object


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

// Convert the piezo vpp to amplitude
const float AUDIO_DRIVER_MIN_VPP = 0.0;  
const float AUDIO_DRIVER_MAX_VPP = 1.44;        // This was measured at volume=1.0 and amplitude=1.0, max value, on oscilliscope
const float AMPLIFIER_GAIN = 100.0;             // Gain of the amplifier
const float MAX_AMPLIFIER_OUTPUT_VPP = 150.0;   // Max output amplitude of the amplifier (+/- VPP/2)

float calculate_amplitude_from_vpp(float piezo_vpp) {
    // Limit the piezo vpp to the max and min values
    if (piezo_vpp > MAX_AMPLIFIER_OUTPUT_VPP) {
        piezo_vpp = MAX_AMPLIFIER_OUTPUT_VPP;
    } else if (piezo_vpp < 0) {
        piezo_vpp = 0;
    }

    // Calculate the input amplitude to the amplifier
    float input_amplitude = piezo_vpp / AMPLIFIER_GAIN;

    // Normalize the amplitude to a value between 0 and 1
    float normalized_amplitude = input_amplitude / AUDIO_DRIVER_MAX_VPP;

    // Limit the normalized amplitude to a value between 0 and 1
    if (normalized_amplitude > 1.0) {
        normalized_amplitude = 1.0;
    } else if (normalized_amplitude < 0.0) {
        normalized_amplitude = 0.0;
    }
    return normalized_amplitude;
}

// Measure the sensors
void measure_sensors() {
    // Measure the flow sensors
    inlet_sensor.measure_flow();
    outlet_sensor.measure_flow();
}

// Save the data just received from the PC into the global variables
void save_data_from_pc() {
    // See if time needs to be reset
	if (pc_to_driver_data.reset_time){
		elapsed_millis = 0;
		elapsed_micros = 0;
        blink_led_state = false;
		// Also reset the blink timer to try and sync with the other MCUs
		statusLEDTimer.reset();
	}

    // Save all other variables to local variables
    signal_type_piezo_1     = pc_to_driver_data.signal_type_piezo_1;
    signal_type_piezo_2     = pc_to_driver_data.signal_type_piezo_2;
    piezo_1_enable          = pc_to_driver_data.piezo_1_enable;
    piezo_2_enable          = pc_to_driver_data.piezo_2_enable;
    piezo_1_freq            = pc_to_driver_data.piezo_1_freq;
    piezo_2_freq            = pc_to_driver_data.piezo_2_freq;
    piezo_1_vpp             = pc_to_driver_data.piezo_1_vpp;
    piezo_2_vpp             = pc_to_driver_data.piezo_2_vpp;
    piezo_1_phase           = pc_to_driver_data.piezo_1_phase;
    piezo_2_phase           = pc_to_driver_data.piezo_2_phase;

    // Calcualte the amplitudes
    piezo_1_amplitude = calculate_amplitude_from_vpp(piezo_1_vpp);
    piezo_2_amplitude = calculate_amplitude_from_vpp(piezo_2_vpp);
}

void send_data_to_pc() {
    // First measure the sensors
    measure_sensors();

    // Write the struct to send to the PC
    driver_to_pc_data.time_ms                   = elapsed_millis;
    driver_to_pc_data.time_us                   = elapsed_micros;
    driver_to_pc_data.signal_type_piezo_1       = signal_type_piezo_1;
    driver_to_pc_data.signal_type_piezo_2       = signal_type_piezo_2;
    driver_to_pc_data.piezo_1_enable            = piezo_1_enable;
    driver_to_pc_data.piezo_2_enable            = piezo_2_enable;
    driver_to_pc_data.piezo_1_freq              = piezo_1_freq;
    driver_to_pc_data.piezo_2_freq              = piezo_2_freq;
    driver_to_pc_data.piezo_1_vpp               = piezo_1_vpp;
    driver_to_pc_data.piezo_2_vpp               = piezo_2_vpp;
    driver_to_pc_data.piezo_1_phase             = piezo_1_phase;
    driver_to_pc_data.piezo_2_phase             = piezo_2_phase;
    driver_to_pc_data.inlet_flow_sensor_ml_min  = inlet_sensor.scaled_flow_value;
    driver_to_pc_data.outlet_flow_sensor_ml_min = outlet_sensor.scaled_flow_value;

    // Send the data to the PC
    ETout_pc.sendData();
}

// -------------------------------- UPDATE THE OUTPUT SIGNAL --------------------------------
// Update a given AudioSynthWaveform object with the new frequency, amplitude, and phase for given piezo number
void update_piezo_synthwaveform(AudioSynthWaveform *waveform, int piezo_number, int signal_type) {
    waveform->begin(signal_type);       // First need to begin the waveform with the correct signal type
    if (piezo_number == 1) {
        waveform->frequency(piezo_1_freq);
        waveform->amplitude(piezo_2_amplitude);
        waveform->phase(piezo_1_phase);
    } else {
        waveform->frequency(piezo_2_freq);
        waveform->amplitude(piezo_2_amplitude);
        waveform->phase(piezo_2_phase);
    }
}

// Update a given AudioSynthNoiseWhite object with the new amplitude for given piezo number
void update_piezo_noise(AudioSynthNoiseWhite *noise, int piezo_number) {
    if (piezo_number == 1) {
        noise->amplitude(piezo_2_amplitude);
    } else {
        noise->amplitude(piezo_2_amplitude);
    }
}

// Update a given AudioSynthToneSweep object with the new frequency, amplitude, and phase for given piezo number
void update_piezo_sweep(AudioSynthToneSweep *sweep, int piezo_number) {
    int sweep_time = 30;        // Number of seconds for the sweep
    int sweep_freq_min = 0;    // Minimum frequency for the sweep
    if (piezo_number == 1) {
        sweep->play(piezo_2_amplitude, sweep_freq_min, piezo_1_freq, sweep_time);      
    } else {
        sweep->play(piezo_2_amplitude, sweep_freq_min, piezo_2_freq, sweep_time);
    }
}


// Update which the type of signal and the parameters of the signal for piezo 1
void update_piezo_1_signal() {
    // First determine if the piezo is enabled
    if (!piezo_1_enable) {
        // Set the mixer gains to 0
        piezo_1_mixer.gain(synthwave_channel, 0);
        piezo_1_mixer.gain(noise_channel, 0);
        piezo_1_mixer.gain(sweep_channel, 0);
        return;
    } 

    // Update the synthwaveform/noise by the type of signal
    if (signal_type_piezo_1 == 4){
        // Update the mixer gains to use the noise waveform
        piezo_1_mixer.gain(synthwave_channel, 0);
        piezo_1_mixer.gain(noise_channel, 1);
        piezo_1_mixer.gain(sweep_channel, 0);
        update_piezo_noise(&piezo_1_noise, 1);
    } else if (signal_type_piezo_1 == 5) {
        // Update the mixer gains to use the sweep waveform
        piezo_1_mixer.gain(synthwave_channel, 0);
        piezo_1_mixer.gain(noise_channel, 0);
        piezo_1_mixer.gain(sweep_channel, 1);
        update_piezo_sweep(&piezo_1_tonesweep, 1);
    } else {
        // Update the mixer gains to use the synthwaveform
        piezo_1_mixer.gain(synthwave_channel, 1);
        piezo_1_mixer.gain(noise_channel, 0);
        piezo_1_mixer.gain(sweep_channel, 0);

        // Since the waveform is either sine, triangle, square, or sawtooth we can use the defined constants in the AudioSynthWaveform library
        update_piezo_synthwaveform(&piezo_1_waveform, 1, signal_type_piezo_1);
    }
}

void update_piezo_2_signal(){
    // First determine if the piezo is enabled
    if (!piezo_2_enable) {
        // Set the mixer gains to 0
        piezo_2_mixer.gain(synthwave_channel, 0);
        piezo_2_mixer.gain(noise_channel, 0);
        piezo_2_mixer.gain(sweep_channel, 0);
        return;
    } 

    // Update the synthwaveform/noise by the type of signal
    if (signal_type_piezo_2 == 4){
        // Update the mixer gains to use the noise waveform
        piezo_2_mixer.gain(synthwave_channel, 0);
        piezo_2_mixer.gain(noise_channel, 1);
        piezo_2_mixer.gain(sweep_channel, 0);
        update_piezo_noise(&piezo_2_noise, 2);
    } else if (signal_type_piezo_2 == 5) {
        // Update the mixer gains to use the sweep waveform
        piezo_2_mixer.gain(synthwave_channel, 0);
        piezo_2_mixer.gain(noise_channel, 0);
        piezo_2_mixer.gain(sweep_channel, 1);
        update_piezo_sweep(&piezo_2_tonesweep, 2);
    } else {
        // Update the mixer gains to use the synthwaveform
        piezo_2_mixer.gain(synthwave_channel, 1);
        piezo_2_mixer.gain(noise_channel, 0);
        piezo_2_mixer.gain(sweep_channel, 0);

        // Since the waveform is either sine, triangle, square, or sawtooth we can use the defined constants in the AudioSynthWaveform library
        update_piezo_synthwaveform(&piezo_2_waveform, 2, signal_type_piezo_2);
    }
}

// -------------------------------- SETUP --------------------------------
void setup(){
    // --------------------------------- EASYTRANSFER SETUP ---------------------------------
    // Begin the serial port (for PC)
    PC_COMMUNICATION.begin(BAUD_RATE);
    ETout_pc.begin(details(driver_to_pc_data), &PC_COMMUNICATION);
    ETin_pc.begin(details(pc_to_driver_data), &PC_COMMUNICATION);

    pinMode(STATUS_PIN, OUTPUT);

    // --------------------------------- FLOW SENSORS SETUP ---------------------------------
    // Begin the flow sensors
    inlet_sensor.begin();
    outlet_sensor.begin();

    // Soft reset the flow sensors
    inlet_sensor.soft_reset();
    outlet_sensor.soft_reset();

    // Set the flow sensor to continuous mode
    inlet_sensor.set_continuous_mode();
    outlet_sensor.set_continuous_mode();

    // --------------------------------- TIMERS SETUP ---------------------------------
    // Setup timers
    statusLEDTimer.setInterval(blink_delay_ms, -1);
    statusLEDTimer.setCallback(blink_status_led);
    statusLEDTimer.start();
    measureSensorsAndSendDataTimer.setInterval(measure_and_send_data_delay_ms, -1);
    measureSensorsAndSendDataTimer.setCallback(send_data_to_pc);        // Measure sensors and send data to PC
    measureSensorsAndSendDataTimer.start();

    // --------------------------------- AUDIO/WAVEFORM SETUP ---------------------------------
    // Init the signals to a sine wave, default values in the config.h file
    
    if (!sgtl5000_1.enable()){
        while (1){
            fast_led_blink();
        }
    };
    sgtl5000_1.volume(1.0);
    AudioMemory(24);
    piezo_1_amplitude = calculate_amplitude_from_vpp(5);
    piezo_2_amplitude = calculate_amplitude_from_vpp(5);
    update_piezo_1_signal();
    update_piezo_2_signal();
    // Note, the piezos should be disabled by default, but just initializing the waveform
};


void loop(){
    // --------------------------- Update the timer objects ---------------------------
    statusLEDTimer.update();
    measureSensorsAndSendDataTimer.update();

    // --------------------------- CHECK EASY TRANSFER IN DATA ---------------------------
	new_data = false;		// Reset each loop, if something depends on new data to update, then put that later in the loop
	if (ETin_pc.receiveData()){
		save_data_from_pc();
		new_data = true;	// Flag to use for anything that needs to be updated when new data is received
	}

    // ------------------- AUDIO DRIVER FOR PIEZOS -------------------
    // If there is new data from the PC, then update the piezo settings
    if (new_data){
        // Update each piezo signal
        for (int i=0; i<10; i++){
            fast_led_blink();
        }
        update_piezo_1_signal();
        update_piezo_2_signal();
    }
};