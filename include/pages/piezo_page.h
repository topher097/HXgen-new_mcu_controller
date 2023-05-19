#ifndef _PIEZO_PAGE_H_
#define _PIEZO_PAGE_H_

#pragma once
#include <Arduino.h>
#include <pageManager.h>                // Brings Simple_GFX, Page, and PageManager classes into scope
#include <config.h>


// class PiezoPage{
// public:
//     PiezoPage(Simple_GFX *tft, TouchScreen *ts) : _tft(tft), _ts(ts){};

//     void initialize();
//     void update();

//     ~PiezoPage(){};


// private:
//     Simple_GFX *_tft;               // Pointer to the tft object
//     TouchScreen *_ts;               // Pointer to the touch screen object

    

//     // Callbacks for the buttons
//     void add_heat_flux_callback();
//     void subtract_heat_flux_callback();
//     void add_inlet_temp_callback();
//     void subtract_inlet_temp_callback();
//     void heaters_on_off_callback();

// };

#endif