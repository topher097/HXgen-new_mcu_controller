#include <Arduino.h>
#include <Wire.h>
#include <EasyTransfer.h>
#include <Timer.h>

// Custom includes
#include <config.h>

// Send data timer
Timer send_data_timer;

// Easy transfer objects
EasyTransfer ETout, ETin;
#define EASY_TRANSFER_SERIAL Serial     // Use the COM port on the Teensy 4.0


// Define the input and output structs
// The input struct needs to be defined as such below to match the python code
struct __attribute__ ((packed)) output_data {
    uint32_t time_ms;
    float sensor;
    uint32_t pc_time_ms_received;
    bool hello_received;
} out_data;

// The output struct needs to be defined as such below to match the python code
struct __attribute__ ((packed)) input_data {
    int32_t pc_time_ms;
    bool hello_flag;
} in_data;


// Blink the LED
void blink_led(){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(25);
    digitalWrite(LED_BUILTIN, LOW);
};

void send_data_callback(){
    // Update the time and random sensor vaiable
    out_data.time_ms = millis();
    out_data.sensor = 1234;

    // Send the data
    ETout.sendData();
    blink_led();

    // Reset the hello flag
    out_data.hello_received = false;
};

// Setup everything
void setup(){
    // Begin the serial port (for PC)
    Serial.begin(BAUDRATE);     // The Baudrate is defined in platformio.ini

    // Begin the EasyTrasnfer objects
    ETin.begin(details(in_data), &EASY_TRANSFER_SERIAL);
    ETout.begin(details(out_data), &EASY_TRANSFER_SERIAL);

    // Set the LED pin as an output
    pinMode(LED_BUILTIN, OUTPUT);

    // Set the send data timer
    send_data_timer.setInterval(1000);                      // Set the interval to 1 second
    send_data_timer.setCallback(send_data_callback);        // Set the callback function
    send_data_timer.start();                                // Start the timer
};


void loop(){
    // Update the send data timer
    send_data_timer.update();

    // Check for new data
    if (ETin.receiveData()){
        // If new data then get the hello flag and save it to the global variable to be written on next send
        out_data.hello_received = in_data.hello_flag;
        out_data.pc_time_ms_received = in_data.pc_time_ms;
    };
};