#include <Arduino.h>
#include <Wire.h>
#include <EasyTransfer.h>
#include <Timer.h>
#include <LiquidCrystal_I2C.h>

// Custom includes
#include <config.h>

// Send data timer
Timer send_data_timer;

// Easy transfer objects
EasyTransfer ETout, ETin;
#define EASY_TRANSFER_SERIAL Serial     // Use the COM port on the Teensy 4.0

// Setup the LCD I2C object for displayign received data
LiquidCrystal_I2C lcd(0x27, 20, 4);         // Address, 20 columns, 4 rows


// Define the input and output structs
// The input struct needs to be defined as such below to match the python code
struct __attribute__ ((packed)) output_data {
    uint32_t time_ms;
    float sensor;
    uint32_t pc_time_ms_received;
    bool hello_received;
    uint8_t checksum_received;
    uint8_t checksum_expected;
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
    out_data.sensor = random(0, 1000);

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

    // Initialize the LCD object
    lcd.init();
    lcd.backlight();

    // Set the LED pin as an output
    pinMode(LED_BUILTIN, OUTPUT);

    // Set the send data timer
    send_data_timer.setInterval(1000);                      // Set the interval to 1 second
    send_data_timer.setCallback(send_data_callback);        // Set the callback function
    send_data_timer.start();                                // Start the timer

    // Write the static LCD characters
    lcd.clear();                // Clear the LCD screen 
    lcd.setCursor(0, 0);
    lcd.print("Rec time ms: "); // Print the time at cursor position 15, 0
    lcd.setCursor(0, 1);
    lcd.print("Rec hello: ");   // Print the hello flag at cursor position 15, 1 
    lcd.setCursor(0, 2);
    lcd.print("Rec chksum: "); // Print the checksum at cursor position 15, 2
    lcd.setCursor(0, 3);
    lcd.print("Calc chksum: "); // Print the checksum at cursor position 15, 3
};


void loop(){
    // Update the send data timer
    send_data_timer.update();

    // Check for new data
    if (ETin.receiveData()){
        // If new data then get the hello flag and save it to the global variable to be written on next send
        out_data.hello_received = in_data.hello_flag;
        out_data.pc_time_ms_received = in_data.pc_time_ms;
        out_data.checksum_received = ETin.get_received_CS();
        out_data.checksum_expected = ETin.get_calced_CS();

        // Update the LCD screen
        uint8_t lcd_row = 0;
        uint8_t lcd_col = 15;
        lcd.setCursor(lcd_col, lcd_row);
        lcd.print(in_data.pc_time_ms);       lcd_row++;
        lcd.setCursor(lcd_col, lcd_row);
        lcd.print(in_data.hello_flag);       lcd_row++;
        lcd.setCursor(lcd_col, lcd_row);
        lcd.print(ETin.get_received_CS());   lcd_row++;
        lcd.setCursor(lcd_col, lcd_row);
        lcd.print(ETin.get_calced_CS());     lcd_row++;

    };
};