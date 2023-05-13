// Include libraries
#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <LiquidCrystal_I2C.h>
#include <Bounce2.h>
#include <sstream>
#include <vector>
#include <EasyTransfer.h>

// Hardware serial (UART) to master
#define MASTER_SERIAL Serial1

// Header files
#include <defaultPiezo.h>

// Namespaces
using namespace defaultPiezoProperties;
using namespace std;

// EasyTransfer objects
EasyTransfer ETin;

// I2C devices
LiquidCrystal_I2C lcd(0x27,20,4);       // Address for LCD

// Special characters for LCD
byte approxCharacter1[8] = {0x00,0x00,0x08,0x15,0x15,0x02,0x00,0x00};
byte approxCharacter2[8] = {0x00,0x00,0x00,0x0A,0x14,0x00,0x00,0x00};
byte degreeCharacter[8] = {0x1C,0x14,0x1C,0x00,0x00,0x00,0x00,0x00};
byte phiCharacter[8] = {0x04,0x04,0x0E,0x15,0x15,0x0E,0x04,0x04};

// Bounce button
Bounce restartButton = Bounce();        // Initialate restart button

// Blink LED
IntervalTimer blinkTimer;   // Timer object for status LED
int ledState = LOW;         
const int blinkDelay = 250; // Blink delay in ms

// LCD update timer
IntervalTimer updateLCDTimer;
const int updateLCDDelay = 1000;

// Pins
// off-limits: 6, 7, 8, 10, 11, 12, 13, 15, 18, 19, 20, 21, 23 
#define RX1     0           // Serial 1 RX
#define TX1     1           // Serial 1 TX
#define BLINK   3           // Status LED pin
#define ENABLE1 16          // Enable driver 1
#define ENABLE2 17          // Enable driver 2
#define RESTART 14          // Restart waveform button

// Audio driver definition
AudioSynthWaveform       waveform1;
AudioSynthWaveform       waveform2; 
AudioOutputI2S           i2s1;          
AudioConnection          patchCord1(waveform1, 0, i2s1, 0);
AudioConnection          patchCord2(waveform2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     

// Struct for communicating with master
struct SLAVE_DATA_STRUCTURE{
  float frequency1;       // Frequency of left channel piezo in Hz
  float frequency2;       // Frequency of right channel piezo in Hz
  float amplitude1;       // Amplitude of sine wave 1 (left cahnnel); 0-1
  float amplitude2;       // Amplitude of sine wave 2 (right channel); 0-1
  float phase1;           // Phase of left channel signal in degrees
  float phase2;           // Phase of right channel signal in degrees
  int enable1;            // Enable pin for piezo driver 1
  int enable2;            // Enable pin for piezo driver 2
};
SLAVE_DATA_STRUCTURE slaveData;

// Blink the status LED 
void blinkLED() {
  if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
  digitalWrite(BLINK, ledState);
  //Serial.println("BLINK");
}

// Reset the piezo properties to the default values
void resetPiezoProperties() {
  slaveData.frequency1 = default_frequency1;    // Frequency of left channel piezo in Hz
  slaveData.frequency2 = default_frequency2;    // Frequency of right channel piezo in Hz
  slaveData.amplitude1 = default_amplitude1;    // Amplitude of sine wave 1 (left cahnnel); 0-1
  slaveData.amplitude2 = default_amplitude2;    // Amplitude of sine wave 2 (right channel); 0-1
  slaveData.phase1 = default_phase1;            // Phase of left channel signal in degrees
  slaveData.phase2 = default_phase2;            // Phase of right channel signal in degrees
  slaveData.enable1 = default_enable1;            // Enable pin for piezo driver 1
  slaveData.enable2 = default_enable2;            // Enable pin for piezo driver 2
}

// When this function is called it updates the sine wave properties with the most updated values,
// so if there is a signal from the master teensy that changes a property of the wave, the output 
// signal for both channels are updated.
void modifySignal(){
  AudioNoInterrupts();
  waveform1.frequency(slaveData.frequency1);              // Frequency of the left channel sine wave; in Hz
  waveform2.frequency(slaveData.frequency2);              // Frequency of the right channel sine wave; in Hz
  waveform1.amplitude(slaveData.amplitude1);              // Amplitude of left channel sine wave; 0-1.0
  waveform2.amplitude(slaveData.amplitude2);              // Amplitude of right channel sine wave; 0-1.0
  waveform1.phase(slaveData.phase1);                      // Phase angle of the left channel wave; 0-360 degrees
  waveform2.phase(slaveData.phase2);                      // Phase angle of the right channel wave; 0-360 degrees
  AudioInterrupts();

  digitalWrite(ENABLE1, slaveData.enable1);           // Enable pin for piezo driver 1
  digitalWrite(ENABLE2, slaveData.enable2);           // Enable pin for piezo driver 2
}

// Update the LCD screen
void updateLCD(){
  if (slaveData.enable1){
    lcd.setCursor(0, 0);
    lcd.print("Piezo 1: " + (String)round(slaveData.frequency1) + "Hz   ");
    lcd.setCursor(0, 1);
    lcd.print("\4" + (String)round(slaveData.phase1) + "\3");
    lcd.setCursor(7, 1);
    lcd.print("\2" + (String)round((slaveData.amplitude1*(3.13/3.3))*driver1Voltage) + "V P-P");
  }
  else {
    lcd.setCursor(0, 0);
    lcd.print("Piezo 1: OFF    ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }

  if (slaveData.enable2){
    lcd.setCursor(0, 2);
    lcd.print("Piezo 2: " + (String)round(slaveData.frequency2) + "Hz   ");
    lcd.setCursor(0, 3);
    lcd.print("\4" + (String)round(slaveData.phase2) + "\3");
    lcd.setCursor(7, 3);
    lcd.print("\2" + (String)round((slaveData.amplitude2*(3.13/3.3))*driver2Voltage) + "V P-P");
  }
  else {
    lcd.setCursor(0, 2);
    lcd.print("Piezo 2: OFF    ");
    lcd.setCursor(0, 3);
    lcd.print("                ");
  }  
}

void setup() {  
  ETin.begin(details(slaveData), &MASTER_SERIAL);       // Serial communication with the master teensy
  
  // Setup piezo properties
  resetPiezoProperties();

  // Pin mode setup
  pinMode(ENABLE1, OUTPUT);
  pinMode(ENABLE2, OUTPUT);
  pinMode(BLINK, OUTPUT);

  // Serial communications
  Serial.begin(115200);             // Serial for USB
  MASTER_SERIAL.begin(115200);      // Serial to connect to master teensy

  // Blink interupt
  blinkTimer.begin(blinkLED, blinkDelay*1000);      // Run the blinkLED function at delay speed
  blinkTimer.priority(1);                           // Priority 0 is highest, 255 is lowest

  // LCD update interupt
  updateLCDTimer.begin(updateLCD, updateLCDDelay*1000);
  updateLCDTimer.priority(2);

  // Begin I2C bus
  Wire.begin();

  // Setup LCD
  lcd.init();
  lcd.backlight();
  lcd.begin(20, 4);
  lcd.createChar(1, approxCharacter1);
  lcd.createChar(2, approxCharacter2);
  lcd.createChar(3, degreeCharacter);
  lcd.createChar(4, phiCharacter);
  updateLCD();

  // Audio driver setup
  AudioMemory(20);
  sgtl5000_1.enable();                          // Enable adio driver
  sgtl5000_1.unmuteLineout();                   // Enable Line Out pins
  sgtl5000_1.lineOutLevel(13);                  // Corresponds to p-p voltage of ~3.13V*amplitude
  waveform1.begin(WAVEFORM_SINE);
  waveform2.begin(WAVEFORM_SINE);
  waveform1.frequency(slaveData.frequency1);              // Frequency of the left channel sine wave; in Hz
  waveform2.frequency(slaveData.frequency2);              // Frequency of the right channel sine wave; in Hz
  waveform1.amplitude(slaveData.amplitude1);              // Amplitude of left channel sine wave; 0-1.0
  waveform2.amplitude(slaveData.amplitude2);              // Amplitude of right channel sine wave; 0-1.0
  waveform1.phase(slaveData.phase1);                      // Phase angle of the left channel wave; 0-360 degrees
  waveform2.phase(slaveData.phase2);                      // Phase angle of the right channel wave; 0-360 degrees

  // Enable the piezo drivers
  digitalWriteFast(ENABLE1, slaveData.enable1);
  digitalWriteFast(ENABLE2, slaveData.enable2);

  // Setup restart button
  restartButton.attach(RESTART, INPUT);     // Attach the debouncer to the pins
  restartButton.interval(25);               // Bounce delay in ms
}

int count = 0;
void loop() {
  restartButton.update();
  if (restartButton.fell()){
    resetPiezoProperties();
    modifySignal();
    updateLCD();
    count = 0;
    Serial.println("Reset signal");
  }

  if (ETin.receiveData()){
    modifySignal();
    Serial.print(count); Serial.println(" Modified signal");
    count ++;
  }
}