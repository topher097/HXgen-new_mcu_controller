#ifndef _tft_PRETTY_MENU_OBJECTS_H_
#define _tft_PRETTY_MENU_OBJECTS_H_


#pragma once
#include <Arduino.h>            // Arduino library for Arduino specific types
#include <config.h>
#include <Adafruit_TFTLCD.h>    // Hardware-specific library
#include <Vector.h>             // Vector library (for the text)
#include <TouchScreen.h>        // Touch screen library (for the buttons)

// Fonts from the gfx library > Fonts folder
#include <Fonts/FreeSansBold12pt7b.h>   // Used for button text
#include <Fonts/FreeSans24pt7b.h>       // Used for title text
#include <Fonts/FreeSans9pt7b.h>        // Used for textbox text

// Colors for the screen
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// Struct for string info
struct StringInfo{
    uint16_t text_size;
    uint16_t width;
    uint16_t height;
};


// Typedef for the String Vector
typedef Vector<String> StringVector;        // Note, using non standard Vector class
typedef Vector<StringVector> StringMatrix;    // Note, using non standard Vector class


// -------------------------------------- UTILS --------------------------------------

// Utils for the menu and other classes
class Utils {
public:
    Utils(){};
    ~Utils(){};

    StringMatrix text_to_vector_of_strings(String *text);          // Note, using non standard Vector class
    StringInfo get_text_info(Adafruit_TFTLCD *tft, String text, uint16_t text_size, GFXfont *font);
    String vector_of_strings_to_string(StringVector *words);    // Note, using non standard Vector class
    StringVector split_textbox_string(Adafruit_TFTLCD *tft, String *text, uint16_t text_size, uint16_t x, uint16_t y, uint16_t box_width, GFXfont *font); 
    bool screen_is_being_touched(TouchScreen *ts);
    uint8_t text_line_gap = 3;         // Gap between lines of text in the button label (if multiple lines) 

private:
    String textbox_string_creator(StringVector *words, uint16_t last_index, uint16_t current_index);    // Note, using non standard Vector class
};


// -------------------------------------- TITLE --------------------------------------

// Create a title object, can be used to display a title on a page
class Title : Utils {
public:
    Title(Adafruit_TFTLCD *tft, TouchScreen *ts) : _tft(tft), _ts(ts){};
    ~Title(){};

private:
    Adafruit_TFTLCD *_tft;
    TouchScreen *_ts;
};


// -------------------------------------- TEXTBOX --------------------------------------

// Creates a textbox object, can be used to display text on a page
class Textbox : Utils {
public:
    Textbox(Adafruit_TFTLCD *tft, TouchScreen *ts) : _tft(tft), _ts(ts){};
    void init_textbox_center_coords(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                                    uint8_t line_gap, uint16_t text_color, uint8_t text_size, String text);
    void init_textbox_top_left_coords(uint8_t x_top_left, uint8_t y_top_left, uint8_t width, uint8_t height, 
                                      uint8_t line_gap, uint16_t text_color, uint8_t text_size, String text);
    ~Textbox(){};

    GFXfont font = FreeSans9pt7b;   // Font to use on textboxes, can override if needed
private:
    Adafruit_TFTLCD *_tft;             // Pointer to the tft object
    TouchScreen *_ts;               // Pointer to the TouchScreen object

    uint16_t _x, _y;                // X and Y coordinate of top left corner
    uint16_t _w, _h;                // Height and Width of textbox rectangle
    uint8_t _text_color;            // Color of textbox text
    uint8_t _text_size;             // Size of textbox text
    String *_text;                  // Text to display in textbox
    
    
    
};


// Creates a button object, can be used to create a button on a page
class Button : Utils {
public:
    Button(Adafruit_TFTLCD *tft, TouchScreen *ts) : _tft(tft), _ts(ts){};

    void init_button_center_coords(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                                   uint16_t fill_color, uint16_t border_color, uint8_t border_width, uint8_t corner_radius, uint8_t border_padding,
                                   uint16_t text_color, uint8_t text_size, String *label);
    void init_button_top_left_coords(uint8_t x_top_left, uint8_t y_top_left, uint8_t width, uint8_t height, 
                                     uint16_t fill_color, uint16_t border_color, uint8_t border_width, uint8_t corner_radius, uint8_t border_padding,
                                     uint16_t text_color, uint8_t text_size, String *label);
    void set_min_max_pressure(uint16_t min_pressure, uint16_t max_pressure);
    void set_min_max_x_y_values(uint16_t min_x, uint16_t max_x, uint16_t min_y, uint16_t max_y);
    void attach_function_callback(void (*callback)(void)){this->pressed_cb = callback;};
    void draw_button(bool button_pressed=false);
    void update_button();       // Used to update the button give touch input, called every loop

    ~Button(){};

    // Set vars, can override if needed
    GFXfont font = FreeSansBold12pt7b;             // Font to use on buttons

protected:
	/*
		Calls the callback function
	*/
	virtual void call();

private:
    Adafruit_TFTLCD *_tft;    // Pointer to the tft object
    TouchScreen *_ts;         // Pointer to the TouchScreen object

    StringMatrix _label;        // Label for button as a StringMatrix type
    uint16_t _x, _y;          // X and Y coordinate of top left corner
    uint16_t _w, _h;          // Height and Width of button rectangle
    uint8_t _fill_color;      // Color of button fill
    uint8_t _text_color;      // Color of button text
    uint8_t _border_color;    // Color of button border
    uint8_t _text_size;       // Size of button text
    uint8_t _corner_radius;   // Rounded corners of button rectangle
    uint8_t _border_padding;  // Number of pixels to pad the button border (just for rendering, creates space around button)
    uint8_t _border_width;    // Width of button border
    volatile uint16_t _t_x, _t_y;      // Coordinates of the touch point
    uint16_t _min_pressure;         // Minimum pressure to register as a touch
    uint16_t _max_pressure;         // Maximum pressure to register as a touch
    uint16_t _min_x, _max_x;        // Minimum and maximum x values to register as a touch
    uint16_t _min_y, _max_y;        // Minimum and maximum y values to register as a touch

    bool curr_touch, last_touch;     // States for if button is touched
    void (*pressed_cb)(void);        // Callback function for when button is pressed


    // Methods
    bool is_pressed(void);

    void print_label();
};


// -------------------------------------- PAGE --------------------------------------

// Create a page object, can be used to create a page on the screen with a title, text, and buttons
class Page : Utils {
public:
    Page(Adafruit_TFTLCD *tft, TouchScreen *ts) : _tft(tft), _ts(ts){};

    void draw_solid_background(uint16_t color);
    void add_buttons(Button *buttons, uint8_t num_buttons);
    void add_title(Title *title);
    void add_textboxes(Textbox *textboxes, uint8_t num_textboxes);

    ~Page(){};

private:
    Adafruit_TFTLCD *_tft;     // Pointer to the tft object
    TouchScreen *_ts;       // Pointer to the TouchScreen object
    Button *_buttons;       // Pointer to the buttons array which contains all the buttons on the page as pointers to the individual button objects
    Title *_title;          // Pointer to the title object
    Textbox *_textboxes;    // Pointer to the textboxes array which contains all the textboxes on the page as pointers to the individual textbox objects

 
};

#endif