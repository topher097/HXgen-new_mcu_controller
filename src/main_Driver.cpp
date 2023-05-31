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
#include <data_transfer.h>
#include <flow_sensor.cpp>
#include <pins_driver.h>


// Easy transfer objects
EasyTransfer ETout, ETin;
#define EASY_TRANSFER_SERIAL Serial1

// Flow sensor objects
const String inlet_string = "inlet";
const String outlet_string = "outlet";
// SoftwareWire inlet_wire(A3, A2);
// SoftwareWire outlet_wire(A8, A0);
FlowSensor inlet_sensor(inlet_string, &Wire);
FlowSensor outlet_sensor(outlet_string, &Wire1);

// Timer for blink
Timer blink_timer;

// // Generic wave form generator
// AudioSynthWaveform       waveform1;      //xy=594,472
// AudioSynthWaveform       waveform2;      //xy=594,549

// // Generic random noise generator
// AudioSynthNoiseWhite     noise1;         //xy=602,633
// AudioSynthNoiseWhite     noise2;         //xy=606,684

// // Tone generator with sweep (sine wave)
// AudioSynthToneSweep      tonesweep1;     //xy=608,814
// AudioSynthToneSweep      tonesweep2;     //xy=605,864

// AudioOutputI2S           i2s2; //xy=908,657
// AudioOutputI2S           i2s3; //xy=915,833
// AudioOutputI2S           i2s1;           //xy=927,506
// AudioConnection          patchCord1(waveform1, 0, i2s1, 0);
// AudioConnection          patchCord2(waveform2, 0, i2s1, 1);
// AudioConnection          patchCord3(noise1, 0, i2s2, 0);
// AudioConnection          patchCord4(tonesweep2, 0, i2s3, 1);
// AudioConnection          patchCord5(noise2, 0, i2s2, 1);
// AudioConnection          patchCord6(tonesweep1, 0, i2s3, 0);
// GUItool: end automatically generated code


// Blink the status LED (callback function for the timer)
void blink_status_led() {
    blink_led_state = !blink_led_state;
    digitalWrite(STATUS_PIN, blink_led_state);
}


void setup(){
    // Begin the serial port (for PC)
    Serial.begin(BAUDRATE);
    EASY_TRANSFER_SERIAL.begin(BAUDRATE);

    //while (!Serial) {}

    // Begin the EasyTrasnfer objects
    ETin.begin(details(monitor_to_driver_data), &EASY_TRANSFER_SERIAL);
    ETout.begin(details(driver_to_monitor_data), &EASY_TRANSFER_SERIAL);

    // Begin the flow sensors
    inlet_sensor.begin();
    outlet_sensor.begin();

    // Soft reset the flow sensors
    inlet_sensor.soft_reset();
    outlet_sensor.soft_reset();

    // Set the flow sensor to continuous mode
    inlet_sensor.set_continuous_mode();
    outlet_sensor.set_continuous_mode();

    // Setup timer
    blink_timer.setInterval(1000, -1);
    blink_timer.setCallback(blink_status_led);
    blink_timer.start();


    pinMode(READ_SENSORS_TRIGGER_PIN, INPUT_PULLUP);

    // waveform1.begin(WAVEFORM_SINE);
    // waveform1.frequency(piezo_1_freq);
    // waveform1.amplitude(1);       // Amplitude requires a value between 0 and 1, with mapping output voltage of 0 to 3.3V
    // waveform1.phase(0);
};


void loop(){
    blink_timer.update();

    // ------------------- EASY TRANSFER STUFF -------------------
    // Read the ETin from the driver to update the piezo settings
    //new_piezo_data = ETin.receiveData();       // If new data from PC, set flag

    // Check for the trigger pin, if it is high, then
    if (digitalRead(READ_SENSORS_TRIGGER_PIN) == HIGH){
        // Read the flow sensors
        inlet_sensor.measure_flow();
        outlet_sensor.measure_flow();

        // Update the ETout struct
        driver_to_monitor_data.inlet_flow_sensor_ml_min = inlet_sensor.scaled_flow_value;
        driver_to_monitor_data.outlet_flow_sensor_ml_min = outlet_sensor.scaled_flow_value;

        // Send the data to the monitor
        ETout.sendData();
        delay(5); // Wait for the data to be sent, don't check the trigger pin too fast
        Serial.print(inlet_sensor.scaled_flow_value); Serial.print(", "); Serial.println(outlet_sensor.scaled_flow_value);
    }


    // ------------------- AUDIO DRIVER FOR PIEZOS -------------------
    // If there is new data from the PC, then update the piezo settings
    if (new_piezo_data){
        piezo_1_freq = monitor_to_driver_data.piezo_1_freq;
        piezo_1_vpp = monitor_to_driver_data.piezo_1_vpp;               // Remember to scale down for the amplifier
        piezo_1_phase = monitor_to_driver_data.piezo_1_phase;
        piezo_2_freq = monitor_to_driver_data.piezo_2_freq;
        piezo_2_vpp = monitor_to_driver_data.piezo_2_vpp;               // Remember to scale down for the amplifier
        piezo_2_phase = monitor_to_driver_data.piezo_2_phase;
        piezo_1_enable = monitor_to_driver_data.piezo_1_enable;
        piezo_2_enable = monitor_to_driver_data.piezo_2_enable;

        // Update the output waveforms
        //waveform1.frequency(piezo_1_freq);
        //waveform1.amplitude(piezo_1_vpp/100);       // Amplitude requires a value between 0 and 1, with mapping output voltage of 0 to 3.3V
    }
};