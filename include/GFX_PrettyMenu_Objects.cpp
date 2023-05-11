#include <tft_PrettyMenu_Objects.h>

// -------------------------------------- UTILS --------------------------------------
// Convert a string to a vector of strings
StringMatrix Utils::text_to_vector_of_strings(String *text){
    // Convert the String to a StringVector type where words are separated by spaces and new lines are denoted as $ 
    // Example:
    // String label = "This is text$with multiple lines";
    // StringVector label_vector = {{"This", "is", "text"}, {"with", "multiple", "lines"}};
    // Note, using non std::vector class
    StringMatrix text_vector;                 // Holds the individual lines as StringVector
    StringVector words;                     // Holds the individual words as String
    StringVector lines;                     // Temporarily holds the lines as StringVector where words are not split by spaces 
    
    // Iterate through all of the characters in the string, and split the String at '$' characters
    uint16_t text_index = 0;
    uint16_t text_length = text->length();
    for (int i=0; i<=text_length; i++){
        if (text->charAt(i)=='$' || i==text_length){
            // Append string to vector
            String sub = text->substring(text_index, i);
            lines.push_back(sub);
            // Get start index for next search
            text_index = i+1;
        }
    }
    
    // Iterate through the lines, and split the String at ' ' characters. Store the words in a StringVector, and then store each line in a StringMatrix
    uint16_t str_index = 0;
    for (int i=0; i<lines.size(); i++){
        // Go line by line and append the words to the words StringVector, and then the words StringVector to the StringMatrix
        String line = lines[i];
        uint16_t line_length = line.length();
        for (int j=0; j<=line_length; j++){
            if (line.charAt(j)==' ' || j==line_length){
                // Append string to vector
                String sub = line.substring(str_index, j);
                words.push_back(sub);
                // Get start index for next search
                str_index = j+1;
            }
        }
        // Append the words to the StringMatrix
        text_vector.push_back(words);
        // Clear the words StringVector for the next line
        words.clear();
        // Reset the str_index
        str_index = 0;
    }

    return text_vector;
};

// Get info about a string given the text, text size, and font
StringInfo Utils::get_text_info(Adafruit_TFTLCD *tft, String text, uint16_t text_size, tftfont *font){
    // Get the width and height of the text
    int16_t x1, y1;
    uint16_t w, h;
    tft->setFont(font);
    tft->setTextSize(text_size);
    tft->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    // Return the text info
    StringInfo text_info = {text_size, w, h};
    return text_info;
};

// Contruct a StringVector from a given String for a text box
StringVector Utils::text_to_textbox_string_vector(Adafruit_TFTLCD *tft, String text, uint16_t text_size, tftfont *font, uint16_t width){
    // Get the text info
    StringInfo text_info = Utils::get_text_info(tft, text, text_size, font);
    // Get the width and height of the text
    uint16_t text_width = text_info.width;
    uint16_t text_height = text_info.height;
    // Get the number of lines
    uint16_t num_lines = text_width/width + 1;
    // Get the number of words
    StringMatrix text_vector = Utils::text_to_vector_of_strings(&text);
    uint16_t num_words = text_vector.size();
    // Get the number of words per line
    uint16_t num_words_per_line = num_words/num_lines + 1;
    // Construct the StringVector
    StringVector textbox_string_vector;
    StringVector words;
    uint16_t word_index = 0;
    for (int i=0; i<num_lines; i++){
        // Get the words for the line
        for (int j=0; j<num_words_per_line; j++){
            // Append the word to the words StringVector
            words.push_back(text_vector[word_index]);
            // Increment the word index
            word_index++;
        }
        // Append the words StringVector to the textbox StringVector
        textbox_string_vector.push_back(words);
        // Clear the words StringVector for the next line
        words.clear();
    }
    return textbox_string_vector;
};

// Construct a String from a given StringVector, where each word in the vector is separated by a space
String Utils::vector_of_strings_to_string(StringVector *words){
    String text = "";
    uint16_t num_words = words->size();
    for (int i=0; i<num_words; i++){
        // Append the word to the text
        text += words->at(i);
        // If not last word, then add a space
        if (i != num_words-1){
            text += " ";
        }
    }
    return text;
};

// Determine if the screen is being touched given the min and max values of the touch screen pressure
bool Utils::is_screen_touched(TouchScreen *ts, uint16_t min_pressure, uint16_t max_pressure){
    // Get the current touch point
    TSPoint touch_point = ts->getPoint();
    // If the touch point is valid
    if (touch_point.z > min_pressure && touch_point.z < max_pressure) {
        // Check if the touch point is within the button bounds
        if (touch_point.x > _x && touch_point.x < _x + _w && touch_point.y > _y && touch_point.y < _y + _h) {
            // If so, then return true
            return true;
        }
    }
    // Otherwise, return false
    return false;
};

// -------------------------------------- PAGE --------------------------------------


// -------------------------------------- TITLE --------------------------------------



// -------------------------------------- TEXTBOX --------------------------------------


// -------------------------------------- BUTTON --------------------------------------
// Button::Button(Adafruit_TFTLCDLCD *tft, TouchScreen *ts){
//     _tft = tft;
//     _ts = ts;
// };


// Initializers
void Button::init_button_center_coords(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                                       uint16_t fill_color, uint16_t border_color, uint8_t border_width, uint8_t corner_radius, uint8_t border_padding,
                                       uint16_t text_color, uint8_t text_size, String *label) {
    // Set button properties
    _x = x - (width/2);           // convert to top left coord
    _y = y - (height/2);          // convert to top left coord
    _w = width;
    _h = height;
    _fill_color = fill_color;
    _border_color = border_color;
    _border_width = border_width;
    _corner_radius = corner_radius;
    _border_padding = border_padding;
    _text_color = text_color;
    _text_size = text_size;
    _label = Utils::text_to_vector_of_strings(label);
}

void Button::init_button_top_left_coords(uint8_t x, uint8_t y, uint8_t width, uint8_t height, 
                                         uint16_t fill_color, uint16_t border_color, uint8_t border_width, uint8_t corner_radius, uint8_t border_padding,
                                         uint16_t text_color, uint8_t text_size, String *label) {
    // Set button properties
    _x = x;
    _y = y;
    _w = width;
    _h = height;
    _fill_color = fill_color;
    _border_color = border_color;
    _border_width = border_width;
    _corner_radius = corner_radius;
    _border_padding = border_padding;
    _text_color = text_color;
    _text_size = text_size;
    _label = Utils::text_to_vector_of_strings(label);
};

// Draw button
void Button::draw_button(bool inverted=false) {
    // Draw button rectangle, then border over it
    uint16_t x = _x + _border_padding;
    uint16_t y = _y + _border_padding;
    uint16_t w = _w - _border_padding*2;
    uint16_t h = _h - _border_padding*2;
    uint16_t b_color, f_color;
    
    // Set the colors
    if (inverted){
        f_color = _border_color;
        b_color = _fill_color;
    } else {
        f_color = _fill_color;
        b_color = _border_color;
    }

    // Draw button rectangle and border
    if (_border_width > 1) {
        // If border width is greater than one then draw rounded rectangle with fill color and then no fill border rectangle
        _tft->fillRoundRect(x, y, w, h, _corner_radius, b_color);    // Draw border first
        _tft->fillRoundRect(x+_border_width, y+_border_width, w-_border_width*2, h-_border_width*2, _corner_radius, f_color);       // Draw fill first
    } else if (_border_width == 1) {
        // If border width is one then draw rounded rectangle with fill color and then no fill border rectangle
        _tft->fillRoundRect(x, y, w, h, _corner_radius, f_color);       // Draw fill first
        _tft->drawRoundRect(x, y, w, h, _corner_radius, b_color);     // Draw border second
    } else {
        // If border width is zero then draw rounded rectangle with fill color and no border rectangle
        _tft->fillRoundRect(x, y, w, h, _corner_radius, f_color);
    }

    // Draw button text
    print_label();
};

// Print button label
void Button::print_label(void) {
    // Set font and text properties
    _tft->setFont(&font);
    _tft->setTextColor(_text_color);
    _tft->setTextSize(_text_size);

    // Get DRAWN button dimensions
    uint16_t x = _x + _border_padding;
    uint16_t y = _y + _border_padding;
    uint16_t w = _w - _border_padding*2;
    uint16_t h = _h - _border_padding*2;

    // Iterate through the StringMatrix and print each string on a new line
    int16_t x1, y1;
    uint16_t w1, h1;
    int num_lines = _label.size();
    // Calculate the vertical offset for the text due to multiple lines
    StringInfo info = Utils::get_text_info(_tft, "H", _text_size, &font);
    int16_t vertical_offset = (info.height * num_lines) / 2 + (text_line_gap * (num_lines-1));      // divide by 2 to get half height, then add line gap for each line

    for (int i = 0; i < num_lines; i++) {
        // Construct a String from the StringVector (aka the line)
        StringVector line = _label[i];
        String line_string = Utils::vector_of_strings_to_string(&line);

        // Get the width and height of the text
        _tft->getTextBounds(line_string, 0, 0, &x1, &y1, &w1, &h1);
        // Print the text (centered due to font not being default font), with vertical offset
        _tft->setCursor(x + (w/2), y + (h/2) + (i*h) - vertical_offset);
        _tft->println(line_string);
    }
};

// Set the min and max pressure values
void Button::set_min_max_pressure(uint16_t min_pressure, uint16_t max_pressure) {
    _min_pressure = min_pressure;
    _max_pressure = max_pressure;
};

// Set the min and max x and y values from touch screen
void Button::set_min_max_x_y(uint16_t min_x, uint16_t max_x, uint16_t min_y, uint16_t max_y) {
    _min_x = min_x;
    _max_x = max_x;
    _min_y = min_y;
    _max_y = max_y;
};

// Update function called every loop. Checks to see if button is pressed and if so, calls the callback function
void Button::update(void) {
    // See of the button is pressed
    bool button_pressed = is_pressed();

    if (button_pressed) {
        // If the button is pressed, then call the callback function
        call();
    }
};


// Calls the callback function
void Button::call() {
	if(pressed_cb != NULL) {
		pressed_cb();
	}
}


// Check to see if the button is currently being pressed
bool Button::is_pressed(void) {
    // Get the current touch point
    TSPoint touch_point = _ts->getPoint();
    // If the touch point is valid
    if (touch_point.z > _min_pressure && touch_point.z < _max_pressure) {
        // If rotation is 0, then x and y are swapped
        _t_x = (uint16_t)map(p.y, _min_x, _max_x, 0, tft.width());     
		_t_y = (uint16_t)map(p.x, _min_y, _max_y, 0, tft->height());

        // Check if the touch point is within the button bounds
        if (touch_point.x > _x && touch_point.x < _x + _w && touch_point.y > _y && touch_point.y < _y + _h) {
            // If so, then return true
            return true;
        }
    }
    // Otherwise, return false
    return false;
};