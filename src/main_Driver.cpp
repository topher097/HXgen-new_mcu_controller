/*
Main file for the Teensy 4.0 that:
    communicates with the Teensy Monitor (4.1) to generate the signal for the piezo amplifier via EasyTransfer,
*/

#include <Arduino.h>
#include <Wire.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// Generic wave form generator
AudioSynthWaveform       waveform1;      //xy=594,472
AudioSynthWaveform       waveform2;      //xy=594,549

// Generic random noise generator
AudioSynthNoiseWhite     noise1;         //xy=602,633
AudioSynthNoiseWhite     noise2;         //xy=606,684

// Tone generator with sweep (sine wave)
AudioSynthToneSweep      tonesweep1;     //xy=608,814
AudioSynthToneSweep      tonesweep2;     //xy=605,864


AudioOutputI2S           i2s2; //xy=908,657
AudioOutputI2S           i2s3; //xy=915,833
AudioOutputI2S           i2s1;           //xy=927,506
AudioConnection          patchCord1(waveform1, 0, i2s1, 0);
AudioConnection          patchCord2(waveform2, 0, i2s1, 1);
AudioConnection          patchCord3(noise1, 0, i2s2, 0);
AudioConnection          patchCord4(tonesweep2, 0, i2s3, 1);
AudioConnection          patchCord5(noise2, 0, i2s2, 1);
AudioConnection          patchCord6(tonesweep1, 0, i2s3, 0);
// GUItool: end automatically generated code


void setup(){



};


void loop(){



};