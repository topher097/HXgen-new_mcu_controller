#ifndef _MAIN_PAGE_H_
#define _MAIN_PAGE_H_

#pragma once
#include <Arduino.h>
#include <pageManager.h>                // Brings Simple_GFX, Page, and PageManager classes into scope
#include <config.h>

// Main page that is displayed when the program starts, has small menu for the piezo and inlet temp pages
template <typename TFTClass>
class MainPage : public Page<TFTClass> {
public: 
    MainPage(PageManager<TFTClass> *manager, Simple_GFX<TFTClass> *sim_gfx, TouchScreen *ts,
             int16_t min_pressure, int16_t max_pressure, 
             int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y) 
             : _manager(manager), _sim_gfx(sim_gfx), _ts(ts), 
             _min_pressure(min_pressure), _max_pressure(max_pressure), 
             _minx(min_x), _maxx(max_x), _miny(min_y), _maxy(max_y){
                font = &FreeSans9pt7b;              // FONT USED FOR STRINGS
                //font = NULL;
             };

    void get_sim_gfx_pointer_addr(void){
        Serial.print(F("MainPage _sim_gfx pointer address: "));
        Serial.println((uintptr_t)_sim_gfx, HEX);
    };

    // Initialize the page, the buttons, and the other objects used in the page
    void initialize(void) {
        // Set the background color
        _sim_gfx->fillScreen(screen_background_color);
        //Serial.println("MainPage background color set");

        // Initialize the page objects
        init_strings();
        init_buttons();
        //Serial.println("MainPage init functions ran");

        // Draw the strings
        draw_static_strings();
        //Serial.println("MainPage draw static strings ran");
        draw_dynamic_strings();
        //Serial.println("MainPage draw dynamic strings ran");

        // Draw the buttons    
        draw_buttons();
    };    

    // Run the individual update functions for the buttons and dynamic strings and other objects
    void update(void) {
        // Update the dynamic strings
        draw_dynamic_strings();

        // Update the button objects
        for (int i = 0; i < num_buttons; ++i) {
            buttons[i]->update();
        };
    };         

    // Initialize the static and dynamic strings
    void init_strings(void){
        // Hardcode the string start positions
        static_string_start_x = 5;
        string_start_y = 5;
        dynamic_string_start_x = 150;
        int16_t dynamic_string_gap = 90;                // Gap for dynamic strings
        int16_t middle_static_string_gap = 80;          // Gap for middle static strings
        static_string_start_x2 = dynamic_string_start_x + dynamic_string_gap;
        dynamic_string_start_x2 = static_string_start_x2 + middle_static_string_gap;
        static_string_start_x3 = dynamic_string_start_x2 + dynamic_string_gap;

        // Get the vertical offset for each line, use the height of the text plus line gap
        text_height = Utils<TFTClass>::get_text_info(_sim_gfx, Utils<TFTClass>::DUMMY_STR, text_size, font).height;
        string_vertical_offset = text_height + line_gap;
    };     

    // Draw the static strings to the screen
    void draw_static_strings(void){
        // Set text attributes
        int16_t x1, y1;
        String temp;

        // Iterate through the first static strings and draw them to the screen
        for (int i = 0; i < num_lines; i++){
            x1 = static_string_start_x;
            y1 = string_start_y + (i * string_vertical_offset);
            temp = first_static_string_lines[i];
            Utils<TFTClass>::print_text_tl_coords(_sim_gfx, x1, y1, temp, string_color, text_size, font);
        }
        //Serial.println("FIRST STATIC STRINGS DRAWN");

        // Iterate through the middle static strings and draw them to the screen
        for (int i = 0; i < num_lines; i++){
            x1 = static_string_start_x2;
            y1 = string_start_y + (i * string_vertical_offset);
            temp = middle_static_string_lines[i];
            Utils<TFTClass>::print_text_tl_coords(_sim_gfx, x1, y1, temp, string_color, text_size, font);
        }
        //Serial.println("MIDDLE STATIC STRINGS DRAWN");

        // Iterate through the end static strings and draw them to the screen
        for (int i = 0; i < num_lines; i++){
            x1 = static_string_start_x3;
            y1 = string_start_y + (i * string_vertical_offset);
            temp = end_static_string_lines[i];
            Utils<TFTClass>::print_text_tl_coords(_sim_gfx, x1, y1, temp, string_color, text_size, font);
        }
        //Serial.println("END STATIC STRINGS DRAWN");
    };  

    // Draw the dynamic strings to the screen, called in the update loop
    void draw_dynamic_strings(void){
         // First fill the background of the dynamic strings to clear any old values with the background color
        // Can optimize this to not draw rectangles in the line gap but this is fine for now
        int16_t x = dynamic_string_start_x;
        int16_t y = string_start_y;
        uint16_t w = static_string_start_x2 - dynamic_string_start_x;
        uint16_t h = string_start_y + (num_lines * string_vertical_offset);
        _sim_gfx->fillRect(x, y, w, h, screen_background_color);
        
        // Iterate through the first dynamic strings and draw them to the screen
        int16_t x_tl, y_tl;
        String temp_string;
        for (int i = 0; i < num_lines; i++){
            x_tl = dynamic_string_start_x;
            y_tl = string_start_y + (i * string_vertical_offset);
            temp_string = String(*first_dynamic_string_variables[i], dynamic_decimal_places);
            Utils<TFTClass>::print_text_tl_coords(_sim_gfx, x_tl, y_tl, temp_string, string_color, text_size, font);
        }

        // Iterate through the second dynamic strings and draw them to the screen, skip the NULL values
        for (int i = 0; i < num_lines; i++){
            x_tl = dynamic_string_start_x2;
            y_tl = string_start_y + (i * string_vertical_offset);
            if (*second_dynamic_string_variables[i] == null){
                temp_string = "";
            } else {
                temp_string = String(*second_dynamic_string_variables[i], dynamic_decimal_places);
            }
            Utils<TFTClass>::print_text_tl_coords(_sim_gfx, x_tl, y_tl, temp_string, string_color, text_size, font);
        }
    };

    // Create the buttons
    void init_buttons(void){
        // Initialze the buttons array
        buttons = new ButtonForMethods<MainPage, TFTClass>*[num_buttons];

        // Iterate through the buttons and initialize them, last button is a toggle type so we do that last
        int16_t x, y, w, h;
        for (int i = 0; i < num_buttons; i++){
            x = button_start_x + (i * button_width);
            y = button_start_y;
            w = button_width;
            h = button_height;

            // Create new button instance
            buttons[i] = new ButtonForMethods<MainPage, TFTClass>(this, _sim_gfx, _ts, &MainPage::goto_inlet_temp_page);

            // Label
            String label = button_labels[i];
            // Print the type of button[i]
            //Serial.print("Button object type: "); Serial.println(typeid(&buttons[i]).name());
            Serial.print("Button sim_gfx address: "); Serial.println(buttons[i]->get_sim_gfx_address(), HEX);

            if (i < num_buttons -1){
                // Initialize the button objects (first 3 buttons for different pages)
                buttons[i]->init_button_top_left_coords(x, y, w, h, 
                                                    button_fill_color, button_outline_color, button_pressed_fill_color, button_outline_color,
                                                    button_outline_thickness, button_corner_radius, button_border_padding,
                                                    button_text_color, button_text_size, &label, Button<TFTClass>::ButtonType::MOMENTARY);
                buttons[i]->set_min_max_pressure(min_pressure, max_pressure);
                buttons[i]->set_min_max_x_y_values(_minx, _maxx, _miny, _maxy);
            } else {
                // Initialize the start stop button with correct colors
                buttons[i]->init_button_top_left_coords(x, y, w, h, 
                                                    start_stop_fill_color, start_stop_outline_color, start_stop_pressed_fill_color, start_stop_outline_color,
                                                    button_outline_thickness, button_corner_radius, button_border_padding,
                                                    button_text_color, button_text_size, &label, Button<TFTClass>::ButtonType::TOGGLE);
            };
        };
    };            

    // Draw the buttons to the screen
    void draw_buttons(void){
        // Draw the buttons to the screen
        for (int i = 0; i < num_buttons; i++){
            Serial.print(F("DRAWING BUTTON: ")); Serial.println(buttons[i]->get_label());
            buttons[i]->draw(false);
        };
        
    };

    ~MainPage(){
        for (int i = 0; i < num_buttons; i++){
            delete buttons[i];
        };
    };

    // BUTTON CALLBACK FUNCTIONS
    void goto_inlet_temp_page(){_manager->set_page(PIEZO_CONFIG_PAGE);};
    void goto_piezo_page(){_manager->set_page(INLET_TEMP_PAGE);};
    void goto_heatflux_page(){_manager->set_page(HEATFLUX_PAGE);};
    void start_stop_toggle(){start_stop_recording = !start_stop_recording;};

private:  
    const GFXfont *font;
    PageManager<TFTClass> *_manager;        // Pointer to the page manager object
    Simple_GFX<TFTClass> *_sim_gfx;             // Pointer to the simple gfx sim_gfx object
    TouchScreen *_ts;                       // Pointer to the touch screen object

    int16_t _min_pressure;          // Minimum pressure to register as a touch
    int16_t _max_pressure;          // Maximum pressure to register as a touch
    int16_t _minx, _maxx;           // Minimum and maximum x values to register as a touch
    int16_t _miny, _maxy;           // Minimum and maximum y values to register as a touch

    uint16_t screen_background_color = BLACK; // The background color of the screen

    float null = -1;                // Keep as -1, this is just to skip lines during printing

    /* Main Page Layout:
    Valve flow rate:    0.0   mL/min
    Inlet flow rate:    0.0   mL/min
    Outlet flow rate:   0.0   mL/min
    Inlet fluid temp:   0.0   C  / 0.0  C                 ( where first value is instantaneous and second is setpoint temperature )
    Piezo 1 freq/V:     0     Hz / 0    V p-p             ( where first value is current frequenct and second is peak to peak voltage )
    Piezo 2 freq/V:     0     Hz / 0    V p-p             ( where first value is current frequenct and second is peak to peak voltage )

    Set Inlet       Piezo       Start/
      Temp          Config       Stop
     Button         Button      Button
    */

    // STRING POSITION AND SIZE VARIABLES
    int16_t text_size;              // The text size for the static/dynamic strings
    int16_t line_gap = 5;           // The gap between each line of text (line padding)
    uint16_t string_color = WHITE;  // The color of the static/dynamic strings
    int16_t static_string_start_x;  // The x position of the start of the static strings
    int16_t string_start_y;         // The y position of the start of the static/dynamic strings, each line adds string height + line gap
    int16_t dynamic_string_start_x; // The x position of the start of the dynamic strings
    int16_t static_string_start_x2; // The x position of the start of the units/middle string
    int16_t dynamic_string_start_x2;// The x position of the start of the second dynamic strings
    int16_t static_string_start_x3; // The x position of the start of the end units string
    int16_t string_vertical_offset; // The vertical offset for each line to be printed
    uint16_t text_height;           // The height of the text in pixels

    // DYNAMIC AND STATIC STRINGS AND ARRAY
    static const int8_t num_lines = 7;              // Number of lines of text to be printed
    int8_t dynamic_decimal_places = 2;              // Number of decimal places to print for dynamic strings
    String first_static_string_lines[num_lines] = {"Valve Flow Rate:", 
                                                    "Inlet Flow Rate:", 
                                                    "Outlet Flow Rate:", 
                                                    "Inlet Fluid Temp:",
                                                    "Heat Flux:", 
                                                    "Piezo 1 Freq/V:", 
                                                    "Piezo 2 Freq/V:"};
    float *first_dynamic_string_variables[num_lines]  = {&save_data.inlet_valve_ml_min, 
                                                        &save_data.inlet_flow_sensor_ml_min, 
                                                        &save_data.outlet_flow_sensor_ml_min, 
                                                        &save_data.inlet_fluid_temp, 
                                                        &save_data.heat_flux,
                                                        &save_data.piezo_1_freq, 
                                                        &save_data.piezo_2_freq};
    String middle_static_string_lines[num_lines] = {"mL/min", 
                                                    "mL/min",
                                                    "mL/min",
                                                    "C  / ", 
                                                    "W/cm^2",
                                                    "Hz / ",
                                                    "Hz / "};
    float *second_dynamic_string_variables[num_lines] = {&null, 
                                                        &null,  
                                                        &null,
                                                        &inlet_fluid_temp_setpoint, 
                                                        &null,
                                                        &save_data.piezo_1_vpp, 
                                                        &save_data.piezo_2_vpp};
    String end_static_string_lines[num_lines] = {"", 
                                                "", 
                                                "", 
                                                "C",
                                                "", 
                                                "V p-p", 
                                                "V p-p"};

    // BUTTON LABELS AND ARRAY
    static const int8_t num_buttons = 4;    // Number of buttons to be created 
    
    // Create a pointer to an array of buttons to easily update in a loop
    ButtonForMethods<MainPage, TFTClass> **buttons;
    String button_labels[num_buttons] = {"Inlet Temp$Settings", 
                                         "Piezo$Config", 
                                         "Heat Flux", 
                                         "Start/$Stop"}; // Labels for the buttons

    // BUTTON SIZE VARIABLES
    float total_button_width_percent_of_screen = 1.0;           // percent of width of screen for buttons to take up
    float total_button_height_percent_of_screen = 0.4;          // percent of height of screen for buttons to take up
    float bottom_screen_button_offset_percent_of_screen = 0.0; // percent of height of screen to offset the buttons from bottom of screen
    const int8_t button_border_padding = 3;
    const int8_t button_text_size = 2;
    const int8_t button_outline_thickness = 1;
    const int8_t button_corner_radius = 3;
    int16_t button_side_gap = _sim_gfx->width() * (1.0-total_button_width_percent_of_screen);
    int16_t button_bottom_gap = _sim_gfx->height() * bottom_screen_button_offset_percent_of_screen; 
    int16_t button_height = _sim_gfx->height() * total_button_height_percent_of_screen;
    int16_t button_width = (_sim_gfx->width() - (2 * button_side_gap)) / num_buttons;
    int16_t button_start_x = button_side_gap;
    int16_t button_start_y = _sim_gfx->height() - button_bottom_gap - button_height;

    // BUTTON COLORS
    const uint16_t button_fill_color = CYAN;                          // Fill for when button is not pressed
    const uint16_t button_pressed_fill_color = BLUE;                  // Fill for when button is pressed
    const uint16_t button_outline_color = BLACK;                      // Outline for button
    const uint16_t button_text_color = BLACK;                         // Text color for button
    const uint16_t start_stop_fill_color = GREEN;                     // Fill for when start/stop button is not pressed
    const uint16_t start_stop_pressed_fill_color = RED;               // Fill for when start/stop button is pressed/toggled
    const uint16_t start_stop_outline_color = BLACK;                  // Outline for start/stop button
    const uint16_t start_stop_text_color = BLACK;                     // Text color for start/stop button            
};

#endif