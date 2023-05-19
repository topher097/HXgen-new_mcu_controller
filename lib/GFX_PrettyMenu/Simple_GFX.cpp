// #include <Simple_GFX_Objects.h>

// // // -------------------------------------- UTILS --------------------------------------
// // // Convert a String to a matrix of StringVectors where each StringVector comprises of words separated by spaces
// // template <typename TFTClass>
// // StringMatrix Utils<TFTClass>::text_to_matrix_of_words(String *text){
// //     // Convert the String to a StringVector type where words are separated by spaces and new lines are denoted as $ 
// //     // Example:
// //     // input : String text = "This is text$with multiple lines";
// //     // output : StringMatrix text_vector = {{"This", "is", "text"}, {"with", "multiple", "lines"}};
// //     //      where text_vector[0] = {"This", "is", "text"} as a StringVector type, and text_vector[1] = {"with", "multiple", "lines"} as a StringVector type 
    
// //     // Split the text into StringVectors where each element is a line of text
// //     StringVector lines = text_to_vector_of_lines(text);

// //     // Convert the StringVector of lines to a StringMatrix of words, where each line is a StringVector of words    
// //     return vector_of_lines_to_matrix_of_words(lines);
// // };

// // // Convert a String to a StringVector where each String is a new line
// // template <typename TFTClass>
// // StringVector Utils<TFTClass>::text_to_vector_of_lines(String *text){
// //      // Iterate through all of the characters in the string, and split the String at '$' characters
// //     // Example:
// //     // String text = "This is text$with multiple lines";
// //     // StringVector lines = {"This is text", "with multiple lines"};
// //     StringVector lines;
// //     int16_t text_index = 0;
// //     int16_t text_length = text->length();
// //     for (int i=0; i<=text_length; i++){
// //         if (text->charAt(i)=='$' || i==text_length){
// //             // Append string to vector
// //             String sub = text->substring(text_index, i);
// //             lines.push_back(sub);
// //             // Get start index for next search
// //             text_index = i+1;
// //         }
// //     }
// //     return lines;
// // };

// // // Convert a String to a StringVector where each element is a word separated by spaces
// // template <typename TFTClass>
// // StringVector Utils<TFTClass>::text_to_vector_of_words(String *text){
// //     // Iterate through all of the characters in the string, and split the String at ' ' characters
// //     // Example:
// //     // String text = "This is text";
// //     // StringVector words = {"This", "is", "text"};
// //     StringVector words;
// //     int16_t text_index = 0;
// //     int16_t text_length = text->length();
// //     for (int i=0; i<=text_length; i++){
// //         if (text->charAt(i)==' ' || i==text_length){
// //             // Append string to vector
// //             String sub = text->substring(text_index, i);
// //             words.push_back(sub);
// //             // Get start index for next search
// //             text_index = i+1;
// //         }
// //     }
// //     return words;
// // };

// // // Convert a StringVector of lines to a StringMatrix of words, where each line is a StringVector of words
// // template <typename TFTClass>
// // StringMatrix Utils<TFTClass>::vector_of_lines_to_matrix_of_words(StringVector lines){
// //     // Iterate through the lines, and split the String at ' ' characters. Store the words in a StringVector, and then store each line in a StringMatrix
// //     // Example:
// //     // StringVector lines = {"This is text", "with multiple lines"};
// //     // StringMatrix text_vector = {{"This", "is", "text"}, {"with", "multiple", "lines"}};
// //     StringVector words;
// //     StringMatrix text_matrix;
// //     String sub, line;
// //     int8_t num_lines = lines.size();

// //     // Go line by line and append the words to the words StringVector, and then the words StringVector to the StringMatrix
// //     for (int16_t i=0; i<num_lines; i++){
// //         line = lines[i];                                        // Extract the line from the StringVector as a String
// //         words = text_to_vector_of_words(&line);                 // Convert the line to a StringVector of words
// //         // Append the words to the StringMatrix
// //         text_matrix.push_back(words);
// //         // Clear the words StringVector for the next line
// //         words.clear();
// //     }
// //     return text_matrix;
// // };

// // // Get info about a string given the text, text size, and font Modifies the info struct defined in header file
// // template <typename TFTClass>
// // StringInfo Utils<TFTClass>::get_text_info(Simple_GFX<TFTClass> *tft, String text, int8_t text_size, const GFXfont *font){
// //     // Get the width and height of the text MAKE SURE FONT YOU USE IS SET BEFORE CALLING THIS FUNCTION
// //     int16_t _x1, _y1;
// //     uint16_t _w1, _h1;
// //     tft->setFont(font);
// //     tft->setTextSize(uint8_t(text_size));
// //     tft->getTextBounds(text, 0, 0, &_x1, &_y1, &_w1, &_h1);
    
// //     // Return the text info
// //     StringInfo text_info;
// //     text_info.text_size = text_size;
// //     text_info.width = _w1;
// //     text_info.height = _h1;
// //     return text_info;
// // };

// // // Contruct a StringVector from a given String for a text box where each element in a StringVector is a line of text that fits the width of the text box
// // template <typename TFTClass>
// // StringVector Utils<TFTClass>::text_to_textbox_string_vector(Simple_GFX<TFTClass> *tft, String *text, uint16_t text_size, uint16_t width, const GFXfont *font){
// //     // Can pass text that has '$' characters to denote new lines, when the new line character is reached, a new line will be started
// //     // Example:
// //     // String text = "This is text$with multiple lines. This is the more text that is long enough to wrap to the next line.";
// //     // StringVector textbox_lines = {"This is text", "with multiple lines. This is the more text", "that is long enough to wrap to the next", "line."};
// //     // Each line must fit in the textbox width

// //     // Get the text as a StringVector where the lines are split at the '$' character. If there is no '$' character, then the StringVector will only have one element
// //     StringVector split_lines = text_to_vector_of_lines(text);

// //     // Iterate through each line and create the longes String that still fits in the width of the textbox. Store each line in a StringVector
// //     StringVector lines;     // Vector of lines that will be returned by the function
// //     String line;
// //     StringVector words;
// //     String word;
// //     int16_t num_words;
// //     uint16_t text_width;
// //     int8_t num_lines = split_lines.size();
// //     for (int8_t i=0; i < num_lines; i++){
// //         line = split_lines[i];                          // Get the line as a String
// //         words = text_to_vector_of_words(&line);         // Get the words in the line as a StringVector
// //         num_words = words.size();                       // Get the number of words in the line
// //         text_width = 0;                                 // Reset the text width
// //         line = "";                                      // Reset the line String
// //         // Iterate through each word in the line and add it to the line String until the width of the line is greater than the width of the textbox
// //         for (int8_t j=0; j<num_words; j++){
// //             word = words[j];                            // Get the word as a String
// //             text_width += get_text_info(tft, word, text_size, font).width;    // Get the width of the word
// //             // If the width of the line is greater than the width of the textbox, then add the line to the lines StringVector and start a new line
// //             if (text_width > width){
// //                 lines.push_back(line);
// //                 line = "";
// //                 text_width = 0;
// //             }
// //             // Add the word to the line
// //             line += word;
// //             // Add a space to the line if it is not the last word in the line
// //             if (j != num_words-1){
// //                 line += " ";
// //             }
// //         }
// //         // Add the last line to the lines StringVector
// //         lines.push_back(line);
// //     }
// //     return lines;
// // };


// // template <typename TFTClass>
// // String Utils<TFTClass>::vector_of_words_to_string(StringVector *words){
// //     String text = "";
// //     int16_t num_words = words->size();
// //     for (int16_t i=0; i<num_words; i++){
// //         // Append the word to the text
// //         text += words->at(i);
// //         // If not last word, then add a space
// //         if (i != num_words-1){
// //             text += " ";
// //         }
// //     }
// //     return text;
// // };

// // // Print a String to the screen given the text, text size, font, x and y coordinates of the TOP LEFT corner of the bounding box wanted
// // // This is used to print text with custom font as the default fonts would print
// // template <typename TFTClass>
// // void Utils<TFTClass>::print_text_tl_coords(Simple_GFX<TFTClass> *tft, int16_t x_tl, int16_t y_tl, String text, uint16_t text_color, int8_t text_size, const GFXfont *font){
// //     // If the font is NULL, then use the default font and use the x and y given to print the String
// //     if (font == NULL){
// //         tft->setFont();
// //         tft->setTextSize(uint8_t(text_size));
// //         tft->setTextColor(text_color);
// //         tft->setCursor(x_tl, y_tl);
// //         tft->println(text);
// //     } else {
// //         // Using custom font, so need to get the text info (height) to offset the y coordinate
// //         StringInfo text_info = get_text_info(tft, text, text_size, font);
// //         // Set the font and text size
// //         tft->setFont(font);
// //         tft->setTextSize(uint8_t(text_size));
// //         tft->setTextColor(text_color);
// //         tft->setCursor(x_tl, y_tl + text_info.height);
// //         tft->println(text);
// //     }
// // };

// // -------------------------------------- TITLE --------------------------------------
// // template <typename TFTClass>
// // void Title<TFTClass>::init_title(uint16_t text_color, int8_t text_size, typename Utils<TFTClass>::TextAlignment alignment, String *text) {
// //     // Set title properties
// //     _text_color = text_color;
// //     _text_size = text_size;
// //     _alignment = alignment;
// //     _text = text;
// // };

// // template <typename TFTClass>
// // void Title<TFTClass>::draw(void){
// //     // Get the text for the title as a Vector of lines
// //     StringVector lines = Utils<TFTClass>::text_to_textbox_string_vector(_tft, _text, _text_size, _tft->width(), font);
// //     int16_t text_height = get_text_info(_tft, DUMMY_STR, _text_size, font).height;  
    
// //     _h = text_height * lines.size() + (lines.size()-1) * text_line_gap;
// //     _w = _tft->width() - 2*side_gap;
// //     _x = side_gap;
// //     _y = top_gap;
// //     bottom_title_y = _y + _h;

// //     // For each line in the title, draw the text to the screen, and make sure it is printed to match to the alignment set
// //     int16_t num_lines = lines.size();
// //     for (int16_t i=0; i<num_lines; i++){
// //         // Get the line
// //         String line = lines.at(i);
// //         // Get the width of the line
// //         int16_t line_width = get_text_info(_tft, line, _text_size, font).width;
// //         // Get the x coordinate of the line based on the alignment
// //         int16_t line_x, line_y;
// //         switch (_alignment) {
// //             case TextAlignment::CENTER:
// //                 line_x = _x + (_w/2) - (line_width/2);
// //                 line_y = _y + (_h/2) - (text_height/2) - ((text_height+text_line_gap) * (num_lines-1)/2) + ((text_height+text_line_gap) * (i-1));
// //                 break;
// //             case TextAlignment::LEFT:
// //                 line_x = _x;
// //                 line_y = _y + ((text_height+text_line_gap) * (i-1));
// //                 break;
// //             case TextAlignment::RIGHT:
// //                 line_x = _x + _w - line_width;
// //                 line_y = _y + ((text_height+text_line_gap) * (i-1));
// //                 break;
// //         }
// //         // Print the line to the screen
// //         print_text_tl_coords(_tft, line_x, line_y, line, _text_color, _text_size, font);
// //     }
// // };

// // template <typename TFTClass>
// // int16_t Title<TFTClass>::get_bottom_y(void){
// //     return bottom_title_y;
// // };


// // -------------------------------------- TEXTBOX --------------------------------------

// // Initializers
// // template <typename TFTClass>
// // void Textbox<TFTClass>::init_textbox_center_coords(int8_t x, int8_t y, int8_t width, int8_t height, 
// //                                          uint16_t fill_color, uint16_t border_color, int8_t border_width, int8_t corner_radius, int8_t border_padding,
// //                                          uint16_t text_color, int8_t text_size, typename Utils<TFTClass>::TextAlignment alignment, String *text) {
// //     // Set textbox properties
// //     _x = x - (width/2);           // convert to top left coord
// //     _y = y - (height/2);          // convert to top left coord
// //     _w = width;
// //     _h = height;
// //     _fill_color = fill_color;
// //     _border_color = border_color;
// //     _border_width = border_width;
// //     _corner_radius = corner_radius;
// //     _border_padding = border_padding;
// //     _text_color = text_color;
// //     _text_size = text_size;
// //     _alignment = alignment;
// //     _text = text;
// // };

// // template <typename TFTClass>
// // void Textbox<TFTClass>::init_textbox_top_left_coords(int8_t x_top_left, int8_t y_top_left, int8_t width, int8_t height, 
// //                                            uint16_t fill_color, uint16_t border_color, int8_t border_width, int8_t corner_radius, int8_t border_padding,
// //                                            uint16_t text_color, int8_t text_size, typename Utils<TFTClass>::TextAlignment alignment, String *text) {
// //     // Set textbox properties
// //     _x = x_top_left;  
// //     _y = y_top_left;  
// //     _w = width;
// //     _h = height;
// //     _fill_color = fill_color;
// //     _border_color = border_color;
// //     _border_width = border_width;
// //     _corner_radius = corner_radius;
// //     _border_padding = border_padding;
// //     _text_color = text_color;
// //     _text_size = text_size;
// //     _alignment = alignment;
// //     _text = text;
// // };

// // // Draw the textbox
// // template <typename TFTClass>
// // void Textbox<TFTClass>::draw(void){
// //     // Draw the textbox shape
// //      if (_border_width > 1) {
// //         // If border width is greater than one then draw rounded rectangle with fill color and then no fill border rectangle
// //         _tft->fillRoundRect(_x, _y, _w, _h, _corner_radius, _border_color);             // Draw border first
// //         _tft->fillRoundRect(_x+_border_width, _y+_border_width, _w-_border_width*2, 
// //                             _h-_border_width*2, _corner_radius, _fill_color);           // Draw fill second
// //     } else if (_border_width == 1) {
// //         // If border width is one then draw rounded rectangle with fill color and then no fill border rectangle
// //         _tft->fillRoundRect(_x, _y, _w, _h, _corner_radius, _fill_color);       // Draw fill first
// //         _tft->drawRoundRect(_x, _y, _w, _h, _corner_radius, _border_color);     // Draw border second
// //     } else {
// //         // If border width is zero then draw rounded rectangle with fill color and no border rectangle
// //         _tft->fillRoundRect(_x, _y, _w, _h, _corner_radius, _fill_color);
// //     }

// //     // Get the text for the textbox as a vector of lines
// //     StringVector lines = text_to_textbox_string_vector(_tft, _text, _text_size, _w, font); 
// //     int16_t text_height = get_text_info(_tft, DUMMY_STR, _text_size, font).height;   

// //     // For each line in the textbox, draw the text to the screen, and make sure it is printed to match to the alignment set
// //     int16_t num_lines = lines.size();
// //     for (int16_t i=0; i<num_lines; i++){
// //         // Get the line
// //         String line = lines.at(i);
// //         // Get the width of the line
// //         int16_t line_width = get_text_info(_tft, line, _text_size, font).width;
// //         // Get the x coordinate of the line based on the alignment
// //         int16_t line_x, line_y;
// //         switch (_alignment) {
// //             case TextAlignment::CENTER:
// //                 line_x = _x + (_w/2) - (line_width/2);
// //                 line_y = _y + (_h/2) - (text_height/2) - (text_height * (num_lines-1)/2) + ((text_height+text_line_gap) * i);
// //                 break;
// //             case TextAlignment::LEFT:
// //                 line_x = _x + _border_padding + _border_width + text_side_padding;
// //                 line_y = _y + _border_padding + _border_width + text_top_padding + ((text_height+text_line_gap) * i);
// //                 break;
// //             case TextAlignment::RIGHT:
// //                 line_x = _x + _w - _border_padding -_border_width - text_side_padding - line_width;
// //                 line_y = _y + _border_padding + _border_width + text_top_padding + ((text_height+text_line_gap) * i);
// //                 break;
// //         }
// //         // Print the line to the screen
// //         print_text_tl_coords(_tft, line_x, line_y, line, _text_size, _text_color, font);
// //     }
// // }; 


// // -------------------------------------- BUTTON --------------------------------------

// // // Initializers
// // template <typename TFTClass>
// // void Button<TFTClass>::init_button_center_coords(int16_t x, int16_t y, int16_t width, int16_t height, 
// //                                          uint16_t fill_color, uint16_t border_color, uint16_t press_fill_color, uint16_t press_border_color,
// //                                          int16_t border_width, int16_t corner_radius, int16_t border_padding,
// //                                          uint16_t text_color, int16_t text_size, String *label, ButtonType type) {
// //     // Set button properties
// //     _x = x - (width/2);           // convert to top left coord
// //     _y = y - (height/2);          // convert to top left coord
// //     _w = width;
// //     _h = height;
// //     _fill_color = fill_color;
// //     _border_color = border_color;
// //     _press_fill_color = press_fill_color;
// //     _press_border_color = press_border_color;
// //     _border_width = border_width;
// //     _corner_radius = corner_radius;
// //     _border_padding = border_padding;
// //     _text_color = text_color;
// //     _text_size = text_size;
// //     _label = text_to_vector_of_lines(label);
// //     _type = type;
// // }

// // template <typename TFTClass>
// // void Button<TFTClass>::init_button_top_left_coords(int16_t x_top_left, int16_t y_top_left, int16_t width, int16_t height, 
// //                                          uint16_t fill_color, uint16_t border_color, uint16_t press_fill_color, uint16_t press_border_color,
// //                                          int16_t border_width, int16_t corner_radius, int16_t border_padding,
// //                                          uint16_t text_color, int16_t text_size, String *label, ButtonType type) {
// //     // Set button properties
// //     _x = x_top_left;
// //     _y = y_top_left;
// //     _w = width;
// //     _h = height;
// //     _fill_color = fill_color;
// //     _border_color = border_color;
// //     _press_fill_color = press_fill_color;
// //     _press_border_color = press_border_color;
// //     _border_width = border_width;
// //     _corner_radius = corner_radius;
// //     _border_padding = border_padding;
// //     _text_color = text_color;
// //     _text_size = text_size;
// //     _label = text_to_vector_of_lines(label);
// //     _type = type;
// // };

// // // Draw button
// // template <typename TFTClass>
// // void Button<TFTClass>::draw(bool pressed) {
// //     // Draw button rectangle, then border over it
// //     int16_t x = _x + _border_padding;
// //     int16_t y = _y + _border_padding;
// //     int16_t w = _w - _border_padding*2;
// //     int16_t h = _h - _border_padding*2;
// //     int16_t r = _corner_radius;
// //     uint16_t b_color, f_color;
    
// //     Serial.println(F("Drawing button")); delay(50);
// //     Serial.print(F("_x: ")); Serial.println(_x); delay(50);
// //     Serial.print(F("_y: ")); Serial.println(_y); delay(50);
// //     Serial.print(F("_w: ")); Serial.println(_w); delay(50);
// //     Serial.print(F("_h: ")); Serial.println(_h); delay(50);
// //     Serial.print(F("_border_padding: ")); Serial.println(_border_padding); delay(50);
// //     Serial.print(F("x: ")); Serial.println(x); delay(50);
// //     Serial.print(F("y: ")); Serial.println(y); delay(50);
// //     Serial.print(F("w: ")); Serial.println(w); delay(50);
// //     Serial.print(F("h: ")); Serial.println(h); delay(50);
    
// //     // Set the colors depending on if the button is pressed/toggled or not
// //     if (pressed){         
// //         f_color = _press_fill_color;
// //         b_color = _press_border_color;
// //     } else {
// //         f_color = _fill_color;
// //         b_color = _border_color;
// //     }
    
// //     Serial.println(F("Colors set")); delay(50); 
    
// //     // Draw button rectangle and border
// //     if (_border_width > 1) {
// //         // If border width is greater than one then draw rounded rectangle with fill color and then no fill border rectangle
// //         _tft->fillRoundRect(x, y, w, h, r, b_color);    // Draw border first
// //         _tft->fillRoundRect(x+_border_width, y+_border_width, w-_border_width*2, h-_border_width*2, r, f_color);       // Draw fill first
// //     } else if (_border_width == 1) {
// //         // If border width is one then draw rounded rectangle with fill color and then no fill border rectangle
// //         Serial.println(F("Drawing button with border width of 1")); delay(50);
// //         _tft->fillRoundRect(x, y, w, h, r, f_color);       // Draw fill first
// //         _tft->drawRoundRect(x, y, w, h, r, b_color);     // Draw border second
// //         delay(5000);
// //     } else {
// //         // If border width is zero then draw rounded rectangle with fill color and no border rectangle
// //         _tft->fillRoundRect(x, y, w, h, r, f_color);
// //     }
    
// //     Serial.println(F("Button drawn")); delay(50);
    

// //     // Draw button text
// //     print_label();
// // };

// // // Print button label
// // template <typename TFTClass>
// // void Button<TFTClass>::print_label(void) {
// //     // Get DRAWN button dimensions
// //     uint16_t x = _x + _border_padding;
// //     uint16_t y = _y + _border_padding;
// //     uint16_t w = _w - _border_padding*2;
// //     uint16_t h = _h - _border_padding*2;

// //     // Iterate through the StringVector and print each string on a new line
// //     int16_t x1, y1;
// //     uint16_t w1, h1;
// //     int16_t x_tl, y_tl;
// //     int8_t num_lines = _label.size();
    
// //     // Calculate the vertical offset for the text due to multiple lines
// //     StringInfo temp_info = get_text_info(_tft, DUMMY_STR, _text_size, font);
// //     int16_t vertical_offset = (temp_info.height * num_lines) / 2 + (text_line_gap * (num_lines-1));      // divide by 2 to get half height, then add line gap for each line

// //     for (int8_t i = 0; i < num_lines; i++) {
// //         // Get the line string
// //         String line_string = _label[i];   
// //         // Get the width and height of the text
// //         _tft->getTextBounds(line_string, 0, 0, &x1, &y1, &w1, &h1);
// //         // Print the text
// //         x_tl = x + (w/2) - (w1/2);
// //         y_tl = y + (h/2) - (h1/2) - ((num_lines-i)/2 * vertical_offset);
// //         print_text_tl_coords(_tft, x_tl, y_tl, line_string, _text_size, _text_color, font);
// //     }
// // };

// // // Set the min and max pressure values
// // template <typename TFTClass>
// // void Button<TFTClass>::set_min_max_pressure(int16_t min_pressure, int16_t max_pressure) {
// //     _min_pressure = min_pressure;
// //     _max_pressure = max_pressure;
// // };

// // // Set the min and max x and y values from touch screen
// // template <typename TFTClass>
// // void Button<TFTClass>::set_min_max_x_y_values(int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y) {
// //     _min_x = min_x;
// //     _max_x = max_x;
// //     _min_y = min_y;
// //     _max_y = max_y;
// // };

// // // Update function called every loop. Checks to see if button is pressed and if so, calls the callback function
// // template <typename TFTClass>
// // void Button<TFTClass>::update(void) {
// //     // See of the button is pressed
// //     curr_touch = is_pressed();

// //     // While button is being pressed, change the colors of the button. For Toggle, this is done when the button is pressed and released
// //     if (curr_touch && !last_touch) {
// //         if (_type == TOGGLE) {
// //             // Do not do anything if the button is toggle button
// //         } else {    
// //             // If the button is not a toggle button (is momentary), then change the colors for pressed state
// //             draw(true);
// //             _pressed = true;
// //         }
// //     }

// //     // If button is pressed and then released
// //     if (!curr_touch && last_touch) {
// //         if (_type == TOGGLE) {
// //             // Update the button colors if the button is a toggle button
// //             _pressed = !_pressed;       // Reverse the press to toggle/untoggle button
// //             draw(true);
// //         } else {
// //             // If the button is not a toggle button (is momentary), then return colors to normal
// //             _pressed = false;
// //             draw(false);
// //         }
// //         call_callback();     // Call the callback function
// //     }
// //     last_touch = curr_touch;
// // };


// // // Check to see if the button is currently being pressed
// // template <typename TFTClass>
// // bool Button<TFTClass>::is_pressed(void) {
// //     // Get the current touch point
// //     TSPoint touch_point = _ts->getPoint();
// //     // If the touch point is valid
// //     if (touch_point.z > _min_pressure && touch_point.z < _max_pressure) {
// //         // If rotation is 0, then x and y are swapped
// //         _t_x = (uint16_t)map(touch_point.y, _min_x, _max_x, 0, _tft->width());     
// // 		_t_y = (uint16_t)map(touch_point.x, _min_y, _max_y, 0, _tft->height());

// //         // Check if the touch point is within the button bounds
// //         if (touch_point.x > _x && touch_point.x < _x + _w && touch_point.y > _y && touch_point.y < _y + _h) {
// //             // If so, then return true
// //             return true;
// //         }
// //     }
// //     // Otherwise, return false
// //     return false;
// // };

// // // Call the callback function and send the button_data struct as an argument
// // // void Button<TFTClass>::call_callback(void) {
// // //     if (function_callback != nullptr){
// // //         function_callback(reinterpret_cast<void*>(&button_data));
// // //     }
// // // };

// // // Call the callback function
// // template <typename TFTClass>
// // void Button<TFTClass>::call_callback(void) {
// //     if (function_callback != nullptr){
// //         function_callback();
// //     }
// // };

// // ---------------------------------- KEYBOARD -------------------------------- //

// // Initialize the numeric keyboard with a String as a target variable
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_alphabetic_keyboard(String *target, int8_t target_char_limit, 
//                                         uint16_t background_color, uint16_t bounding_box_color, uint16_t bounding_box_border_color,
//                                         uint16_t button_fill_color, uint16_t button_border_color, uint16_t button_text_color) {
//     // Set the target variable
//     _target_type = target_type::STRING;
//     _alph_target = target;
//     _target_char_limit = target_char_limit;

//     // Set the colors
//     _background_color = background_color;
//     _bounding_box_color = bounding_box_color;
//     _button_fill_color = button_fill_color;
//     _button_border_color = button_border_color;
//     _button_text_color = button_text_color;

//     // Initialize the keyboard bounding box, key objects, and textbox object
//     init_keyboard_boundary();
//     init_textbox();
//     init_alphabetic_keys();

//     // Draw the initial graphics for the bounding box, keys, and textbox
//     init_draw_keyboard_boundary();
//     init_draw_textbox();
//     init_alphabetic_keys();           
// };

// // Initialize the numeric keyboard with a float as a target variable
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_numeric_keyboard(float *target, float min_target_value, float max_target_value, 
//                                      uint16_t background_color, uint16_t bounding_box_color, uint16_t bounding_box_border_color,
//                                      uint16_t button_fill_color, uint16_t button_border_color, uint16_t button_text_color) {
//     // Set the target variable
//     _target_type = target_type::FLOAT;
//     _float_target = target;
//     _min_target_value = min_target_value;
//     _max_target_value = max_target_value;

//     // Set the colors
//     _background_color = background_color;
//     _bounding_box_color = bounding_box_color;
//     _bounding_box_border_color = bounding_box_border_color;
//     _button_fill_color = button_fill_color;
//     _button_border_color = button_border_color;
//     _button_text_color = button_text_color;

//     // Initialize the keyboard bounding box, key objects, and textbox object
//     init_keyboard_boundary();
//     init_textbox();
//     init_numeric_keys();

//     // Draw the initial graphics for the bounding box, keys, and textbox
//     init_draw_keyboard_boundary();
//     init_draw_textbox();
//     init_draw_numeric_keys();
// };

// // Initialize the numeric keyboard with a int as a target variable
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_numeric_keyboard(int *target, int min_target_value, int max_target_value, 
//                                      uint16_t background_color, uint16_t bounding_box_color, uint16_t bounding_box_border_color,
//                                      uint16_t button_fill_color, uint16_t button_border_color, uint16_t button_text_color) {
//     // Set the target variable
//     _target_type = target_type::INTEGER;
//     _int_target = target;
//     _min_target_value = (float)min_target_value;
//     _max_target_value = (float)max_target_value;

//     // Set the colors
//     _background_color = background_color;
//     _bounding_box_color = bounding_box_color;
//     _bounding_box_border_color = bounding_box_border_color;
//     _button_fill_color = button_fill_color;
//     _button_border_color = button_border_color;
//     _button_text_color = button_text_color;

//     // Initialize the keyboard bounding box, key objects, and textbox object
//     init_keyboard_boundary();
//     init_textbox();
//     init_numeric_keys();

//     // Draw the initial graphics for the bounding box, keys, and textbox
//     init_draw_keyboard_boundary();
//     init_draw_textbox();
//     init_draw_numeric_keys();
// };

// // Draw the keyboard bounding box
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_draw_keyboard_boundary(void) {
//     // Draw the Fill boundary, then the Border
//     _tft->fillRoundRect(kb_tl_x, kb_tl_y, total_width, total_height, bounding_box_corner_radius, _bounding_box_color);
//     _tft->drawRoundRect(kb_tl_x, kb_tl_y, total_width, total_height, bounding_box_corner_radius, _bounding_box_border_color);

//     // Draw fill rectangle and border
//     if (bounding_box_border_width > 1) {
//         // If border width is greater than one then draw rounded rectangle with fill color and then no fill border rectangle
//         _tft->fillRoundRect(kb_tl_x, kb_tl_y, total_width, total_height, bounding_box_corner_radius, _bounding_box_border_color);       // Draw border first
//         _tft->fillRoundRect(kb_tl_x+bounding_box_border_width, kb_tl_y+bounding_box_border_width, total_width-bounding_box_border_width*2, 
//                             total_height-bounding_box_border_width*2, bounding_box_corner_radius, _bounding_box_color);                 // Draw fill second
//     } else if (bounding_box_border_width == 1) {
//         // If border width is one then draw rounded rectangle with fill color and then no fill border rectangle
//         _tft->fillRoundRect(kb_tl_x, kb_tl_y, total_width, total_height, bounding_box_corner_radius, _bounding_box_color);              // Draw fill first
//         _tft->drawRoundRect(kb_tl_x, kb_tl_y, total_width, total_height, bounding_box_corner_radius, _bounding_box_border_color);       // Draw border second
//     } else {
//         // If border width is zero then draw rounded rectangle with fill color and no border rectangle
//         _tft->fillRoundRect(kb_tl_x, kb_tl_y, total_width, total_height, bounding_box_corner_radius, _bounding_box_color);
//     }
// };

// // Draw the textbox
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_draw_textbox(void) {
//     // Call the textbox object draw function
//     _textbox->draw();
// };

// // Draw the numeric key objects
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_draw_numeric_keys(void) {
//     // Draw the buttons
//     for (int i = 0; i < num_keys; i++) {
//         if (_buttons_numeric[i] != NULL) {
//             _buttons_numeric[i]->draw();
//         }
//     }
// };

// // Draw the alphabetic key objects
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_draw_alphabetic_keys(void) {
//     // Draw the buttons
//     for (int i = 0; i < num_keys; i++) {
//         if (_buttons_alphabetic[i] != NULL) {
//             _buttons_alphabetic[i]->draw();
//         }
//     }
// };


// // Initialize the keyboard boundary
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_keyboard_boundary(void) {
//     // Calc the keyboard width, height, and top left x and y
//     total_width = keyboard_width_percent * _tft->width();
//     total_height = (keyboard_height_percent + key_input_text_box_height_percent) * _tft->height();
//     kb_tl_x = (_tft->width() - total_width) / 2;
//     kb_tl_y = (_tft->height() - total_height) / 2;
// };

// // Initialize the text box for the input
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_textbox(void) {
//     // Calculate the width, height, x, and y of the text box
//     tb_w = total_width;
//     tb_h = key_input_text_box_height_percent * _tft->height();
//     tb_tl_x = kb_tl_x;
//     tb_tl_y = kb_tl_y;
// };


// // Initialize the numeric keys and save the objects to the _buttons_numeric array
//  // 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ., backspace, enter
// template <typename TFTClass>
// void Keyboard<TFTClass>::init_numeric_keys(void) {
//     // This keyboard will have this layout:
//     /*
//           "input string textbox"
//            1        2       3 
//            4        5       6
//            7        8       9
//            .        0      DEL
//                   ENTER 
//     */

//     // Initialize the numeric key constants
//     uint16_t x, y;
//     num_rows = 5;
//     num_cols = 3;
//     num_keys = 13;
//     key_width = total_width / num_cols;
//     key_height = (total_height - tb_h) / num_rows;
//     String labels[num_keys] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "<-", "ENTER"};
//     String *label;

//     // Create the first 4 rows of keys, last row with ENTER will be done separately
//     for (int i = 0; i < num_rows-1; i++) {
//         for (int j = 0; j < num_cols; j++) {
//             // Calculate the x and y of the key (top left corner)
//             x = kb_tl_x + j * key_width;
//             y = kb_tl_y + i * key_height + tb_h;

//             // Create the key
//             label = &labels[i*num_cols + j];        // Get pointer to the label String
//             Button *key = new Button(_tft, _ts);
//             key->init_button_top_left_coords(x, y, key_width, key_height, 
//                                              _button_fill_color, _button_border_color, _button_press_fill_color, _button_border_color,
//                                             key_border_width, key_corner_radius, object_border_padding, 
//                                              _button_text_color, key_text_size, label, typename Button<TFTClass>::ButtonType::MOMENTARY);
//             // Add the key to the array
//             _buttons_numeric[i*num_cols + j] = key;
//         }
//     }

//     // Create the ENTER button
//     x = kb_tl_x;                                            // Full width of the boundary
//     y = kb_tl_y + (num_rows-1) * key_height + tb_h;         // Same height as the other keys
//     key_width = total_width;                                // Full width of the boundary
//     key_height = key_height;                                // Same height as the other keys
//     label = &labels[num_keys-1];                            // Get pointer to the label String
//     Button *key = new Button(_tft, _ts);
//     key->init_button_top_left_coords(x, y, key_width, key_height, 
//                                      _button_fill_color, _button_border_color, _button_press_fill_color, _button_border_color, 
//                                      key_border_width, key_corner_radius, object_border_padding, 
//                                      _button_text_color, key_text_size, label, typename Button<TFTClass>::ButtonType::MOMENTARY);
//     // Add the key to the array
//     _buttons_numeric[num_keys-1] = key;
// };
