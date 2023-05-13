/*
Main file for the Teensy 4.1 that:
    measures all 14 thermistors,
    measures the 2 flow sensors using I2C,
    measures the 2 inlet valve and 1 potentiometer and calculates the inlet flow rate across valve,
    communicates with the MEGA and receives the heat flux and inlet temperatures via EasyTransfer,
    communicates with the Driver Teensy to send the signals to the piezo amplifier via EasyTransfer,
    logs all of the data to the SD card
*/

#include <Arduino.h>
#include <Wire.h>
