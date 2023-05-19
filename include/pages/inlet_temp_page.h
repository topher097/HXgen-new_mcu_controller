#ifndef _INLET_TEMP_PAGE_H_
#define _INLET_TEMP_PAGE_H_

#pragma once
#include <Arduino.h>
#include <pageManager.h>                // Brings Simple_GFX, Page, and PageManager classes into scope
#include <config.h>

// Inlet temp page that is displayed when the program starts, has small menu for the piezo and inlet temp pages
template <typename TFTClass>
class InletTempPage : public Page<TFTClass> {
public: 
    InletTempPage(PageManager<TFTClass> *manager, Simple_GFX<TFTClass> *tft, TouchScreen *ts,
             int16_t min_pressure, int16_t max_pressure, 
             int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y) 
             : _manager(manager), _tft(tft), _ts(ts), 
             _min_pressure(min_pressure), _max_pressure(max_pressure), 
             _minx(min_x), _maxx(max_x), _miny(min_y), _maxy(max_y),
             add_button(this, tft, ts, &InletTempPage::add_inlet_temp),
             subtract_button(this, tft, ts, &InletTempPage::subtract_inlet_temp),
             back_button(this, tft, ts, &InletTempPage::goto_previous_page){
                font = &FreeSans9pt7b;
                buttons[0] = &add_button; 
                buttons[1] = &subtract_button;
                buttons[2] = &back_button;
                inlet_temp_setpoint = &inlet_fluid_temp_setpoint;       // Set the pointer
            };
   
    // ----------------------------- CLASS METHODS -----------------------------
    // Initialize the page, the buttons, and the other objects used in the page
    void initialize(void){
        // Set the background color
        _tft->fillScreen(screen_background_color);

        // Initialize the title, strings, and buttons
        init_title();
        init_strings();
        init_buttons();

        // Draw the strings to the screen
        draw_static_strings();
        draw_dynamic_strings();

        // Draw buttons to the screen
        draw_buttons();
    };

    // Run the individual update functions for the buttons and dynamic strings and other objects
    void update(void){
        // Update the dynamic strings
        draw_dynamic_strings();

        // Update the button objects
        for (int i = 0; i < num_buttons; i++){
            buttons[i]->update();
        }
    };     

    ~InletTempPage(){};

    // ----------------------------- BUTTON CALLBACK FUNCTIONS -----------------------------
    void goto_previous_page(){
        _manager->goto_previous_page(); 
    };
    void add_inlet_temp(){
        // Change the value of inlet_fluid_temp_setpoint found in config.h
        inlet_fluid_temp_setpoint += inlet_temp_setpoint_increment;
    };          
    void subtract_inlet_temp(){
        // Change the value of inlet_fluid_temp_setpoint found in config.h
        inlet_fluid_temp_setpoint -= inlet_temp_setpoint_increment;
    };

    // ----------------------------- PAGE OBJECT INITIALIZATION ----------------------------
    void init_title(){
        // Create and initialize the title object, but don't draw yet
        Title<TFTClass> *title = new Title<TFTClass>(_tft);
        title->init_title(title_text_color, title_text_size, Utils<TFTClass>::CENTER, &title_label);
    };

    void init_buttons(void){
        // Initialize the button objects
        int16_t x, y;
        uint16_t w, h;

        // Calculate the starting x and y positions for the buttons on the first row
        int16_t button_start_x = button_side_gap;
        int16_t button_start_y = _tft->height() - button_bottom_gap - button_height;  

        // Initialize the buttons on the first row
        for (int i = 0; i < num_button_cols; i++){
            x = button_start_x + (i * button_width);
            y = button_start_y;
            w = button_width;
            h = button_height;
            buttons[i]->init_button_top_left_coords(x, y, w, h, 
                                                button_fill_color, button_outline_color, button_pressed_fill_color, button_outline_color,
                                                button_outline_thickness, button_corner_radius, button_border_padding,
                                                button_text_color, button_text_size, &button_labels[i], Button<TFTClass>::ButtonType::MOMENTARY);
            buttons[i]->set_min_max_pressure(min_pressure, max_pressure);
            buttons[i]->set_min_max_x_y_values(_minx, _maxx, _miny, _maxy);

        }

        // Initialize the button on the second row
        button_start_y = button_start_y - button_height;
        buttons[num_buttons]->init_button_top_left_coords(button_start_x, button_start_y, button_width, button_height, 
                                            button_fill_color, button_outline_color, button_pressed_fill_color, button_outline_color,
                                            button_outline_thickness, button_corner_radius, button_border_padding,
                                            button_text_color, button_text_size, &button_labels[num_buttons], Button<TFTClass>::ButtonType::MOMENTARY);
        buttons[num_buttons]->set_min_max_pressure(min_pressure, max_pressure);
        buttons[num_buttons]->set_min_max_x_y_values(_minx, _maxx, _miny, _maxy);
    };

    void init_strings(){
        // Hardcode the string start positions
        static_string_start_x = 10;
        dynamic_string_start_x = 200;
        int16_t dynamic_string_gap = 60;                // Gap for dynamic strings
        static_string_start_x2 = dynamic_string_start_x + dynamic_string_gap;

        // Calculate the y position of the string line, center it between bottom of title and top of buttons
        string_vertical_offset = get_text_info(_tft, Utils<TFTClass>::DUMMY_STR, string_text_size, font).height;
        
        int16_t bottom_title = title->get_bottom_y();
        int16_t top_buttons = total_button_height_percent_of_screen * _tft->height() + button_bottom_gap;

        int16_t string_height = get_text_info(_tft, Utils<TFTClass>::DUMMY_STR, string_text_size, font).height;
        int16_t total_gap_pixels = top_buttons - bottom_title;
        string_start_y = bottom_title + (total_gap_pixels / 2) - (string_height / 2); 
    };


    // ----------------------------- PAGE OBJECT DRAWING -----------------------------
    void draw_static_strings(void){
        // Set text attributes
        _tft->setTextSize(string_text_size);
        _tft->setTextColor(string_color);
        _tft->setFont(font);

        // Print the static strings to page
        _tft->setCursor(static_string_start_x, string_start_y);
        _tft->println(static_string_labels[0]);
        _tft->setCursor(static_string_start_x2, string_start_y);
        _tft->println(static_string_labels[1]);    
    };

    void draw_dynamic_strings(void){
        // Set text attributes
        _tft->setTextSize(string_text_size);
        _tft->setTextColor(string_color);
        _tft->setFont(font);

        // First fill the background of the dynamic strings to clear any old values with the background color
        int16_t x = dynamic_string_start_x;
        int16_t y = string_start_y;
        uint16_t w = static_string_start_x2 - dynamic_string_start_x;
        uint16_t h = string_start_y + string_vertical_offset;
        _tft->fillRect(x, y, w, h, screen_background_color);
        
        // Print the dynamic string to page
        _tft->setCursor(dynamic_string_start_x, string_start_y);
        _tft->println(String(*dynamic_string_variables[0], dynamic_string_decimal_places));
    };

    void draw_buttons(void){
        // Draw the buttons to the screen
        for (int i = 0; i < num_buttons; i++){
            buttons[i]->draw();
        }
    };

private:  
    const GFXfont *font;
    float *inlet_temp_setpoint;         // Pointer to the inlet_fluid_temp_setpoint
    PageManager<TFTClass> *_manager;    // Pointer to the page manager object
    Simple_GFX<TFTClass> *_tft;         // Pointer to the tft object
    TouchScreen *_ts;                   // Pointer to the touch screen object

    int16_t _min_pressure;          // Minimum pressure to register as a touch
    int16_t _max_pressure;          // Maximum pressure to register as a touch
    int16_t _minx, _maxx;           // Minimum and maximum x values to register as a touch
    int16_t _miny, _maxy;           // Minimum and maximum y values to register as a touch

    uint16_t screen_background_color = BLACK;   // The background color of the screen

    float inlet_temp_setpoint_increment = 0.5;  // The amount to increment the inlet temp setpoint by

    // PAGE LAYOUT
    /*  INLET FLUID TEMPERATURE                     ( This is a multiline Title object)
                SETPOINT

        INLET FLUID TEMP:  XX.X C                   ( This is a static string, then dynamic, then static string)

        +                 -                         ( Two buttons, spanning full width of screen)

                BACK                                ( One button, spanning full width of screen)
    */

    // STRING POSITION AND SIZE VARIABLES
    int8_t string_text_size = 2;    // The text size for the static/dynamic strings
    int8_t line_gap = 5;            // The gap between each line of text (line padding)
    uint16_t string_color = WHITE;  // The color of the static/dynamic strings
    int16_t string_start_y;         // The y position of the start of the static/dynamic strings, each line adds string height + line gap
    int16_t static_string_start_x;  // The x position of the start of the static strings
    int16_t dynamic_string_start_x; // The x position of the start of the dynamic strings
    int16_t static_string_start_x2; // The x position of the start of the units
    int16_t string_vertical_offset; // The vertical offset of line of strings (height of text plus line gap)

    // DYNAMIC AND STATIC STRINGS AND ARRAY
    static const int8_t num_static_strings = 2;                 // Number of static strings to be created
    String static_string_labels[num_static_strings] = {"Temperature:", "C"};
    float *dynamic_string_variables[1] = {inlet_temp_setpoint}; // Array of pointers to the dynamic variables
    int8_t dynamic_string_decimal_places = 1;                   // Number of decimal places to display for the dynamic strings

    // BUTTON LABELS AND ARRAY
    static const int8_t num_buttons = 3;                        // Number of buttons to be created 
    static const int8_t num_button_rows = 2;                    // Number of rows of buttons to be created
    static const int8_t num_button_cols = 2;                    // Number of columns of buttons to be created
    Button<TFTClass> *buttons[num_buttons];                               // Create a pointer to an array of buttons to easily update in a loop
    ButtonForMethods<InletTempPage, TFTClass> add_button;                 // Create a button for the add_inlet_temp method
    ButtonForMethods<InletTempPage, TFTClass> subtract_button;            // Create a button for the subtract_inlet_temp method
    ButtonForMethods<InletTempPage, TFTClass> back_button;                // Create a button for the goto_previous_page method
    String button_labels[num_buttons] = {"+", "-", "BACK"};     // Labels for the buttons

    // BUTTON SIZE VARIABLES
    float total_button_width_percent_of_screen = 1.0;           // percent of width of screen for buttons to take up
    float total_button_height_percent_of_screen = 0.45;         // percent of height of screen for buttons to take up
    float bottom_screen_button_offset_percent_of_screen = 0.05; // percent of height of screen to offset the buttons from bottom of screen
    const int8_t button_border_padding = 3;
    const int8_t button_text_size = 3;
    const int8_t button_outline_thickness = 1;
    const int8_t button_corner_radius = 3;
    int16_t button_side_gap         = _tft->width() * total_button_width_percent_of_screen;
    int16_t button_bottom_gap       = _tft->height() * bottom_screen_button_offset_percent_of_screen; 
    int16_t button_height           = _tft->height() * total_button_height_percent_of_screen / num_button_rows;
    int16_t button_width            = (_tft->width() - (2 * button_side_gap)) / num_button_cols;        // Not width of "BACK" button
    int16_t button_start_x;
    int16_t button_start_y;  

    // BUTTON COLORS
    const uint16_t button_fill_color = CYAN;                          // Fill for when button is not pressed
    const uint16_t button_pressed_fill_color = GREEN;                 // Fill for when button is pressed
    const uint16_t button_outline_color = BLACK;                      // Outline for button
    const uint16_t button_text_color = BLACK;                         // Text color for button

    // TITLE
    Title<TFTClass> *title;                                     // Pointer to the title object
    String title_label = "INLET FLUID TEMPERATURE$SETPOINT";    // Label for the title (multiline)
    const int8_t title_text_size = 2;                           // The text size for the title
    const uint16_t title_text_color = WHITE;                    // The color of the title text
};

#endif