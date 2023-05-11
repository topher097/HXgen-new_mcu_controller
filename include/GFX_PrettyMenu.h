#ifndef _GFX_PRETTY_MENU_H_
#define _GFX_PRETTY_MENU_H_


#pragma once
#include <Arduino.h>            // Arduino library for Arduino specific types
#include <config.h>
#include <Adafruit_GFX.h>       // Core graphics library
#include <Adafruit_TFTLCD.h>    // Hardware-specific library
#include <Vector.h>             // Vector library (for the text)
#include <TouchScreen.h>        // Touch screen library (for the buttons)
#include <GFX_PrettyMenu_Objects.cpp>


struct PageMapping{
    uint8_t page_num;       // page number
    Page* page;             // un-typed pointer to the Page object
};

typedef Vector<PageMapping> PageMap;        // Vector of PageMapping structs ({page_num, Page}) used in the menu and sub menus. Note, using non standard Vector class



// -------------------------------------- GFX_PrettyMenu --------------------------------------

// Wrapper for the Adafruit_TFTLCD class, adds some more functionality and makes code cleaner in main.cpp
class GFX_PrettyMenu : Utils { 
public:
    GFX_PrettyMenu(Adafruit_GFX *gfx, TouchScreen *ts, PageMap *page_map){
        _gfx = gfx;
        _ts = ts;
        _page_map = page_map;
    };
    void initialize();
    void update();
    ~GFX_PrettyMenu(){};

private:
    Adafruit_GFX *_gfx;      // Pointer to the GFX object, can be initialized as Adafruit_TFTLCD or another GFX object
    TouchScreen *_ts;        // Pointer to the TouchScreen object
    PageMap *_page_map;      // Pointer to the PageMap vector which contains PageMapping structs where all the page is a pointer to the individual Page object and page_num corresponds to that page number
    uint8_t current_page;    // Current page number
    uint8_t previous_page;   // Previous page number
};


#endif