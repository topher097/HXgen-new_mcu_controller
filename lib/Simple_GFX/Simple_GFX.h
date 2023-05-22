#ifndef _SIMPLE_GFX_H_
#define _SIMPLE_GFX_H_


#pragma once
#include <Arduino.h>            // Arduino library for Arduino specific types
#include <Adafruit_GFX.h>       // Core graphics library
#ifdef USE_TFTLCD               // Set in your main.cpp file, depending on which screen you're using
    #include <Adafruit_TFTLCD.h>    // Hardware-specific library for 8bit mode
#else
    #include <Adafruit_SPITFT.h>    // Hardware-specific library for SPI mode
#endif
#include <Vector.h>             // Vector library (for the text)
#include <TouchScreen.h>        // Touch screen library (for the buttons)
#include <typeinfo>

// Fonts from the gfx library > Fonts folder
#include <Fonts/FreeSansBold12pt7b.h>   // Used for button text
#include <Fonts/FreeSans24pt7b.h>       // Used for title text
#include <Fonts/FreeSans9pt7b.h>        // Used for textbox text

/*
Using custom fonts, the x and y coordinates are the BOTTOM LEFT corner of the text

Using the defaul (NULL) font, the x and y coordinates are the TOP LEFT corner of the text
*/

// Colors for the screen
static const uint16_t BLACK      = 0x0000;
static const uint16_t BLUE       = 0x001F;
static const uint16_t RED        = 0xF800;
static const uint16_t GREEN      = 0x07E0;
static const uint16_t CYAN       = 0x07FF;
static const uint16_t MAGENTA    = 0xF81F;
static const uint16_t YELLOW     = 0xFFE0;
static const uint16_t WHITE      = 0xFFFF;

// Typedef for the String Vector
typedef String String;
typedef Vector<String> StringVector;          // Note, using non standard Vector class
typedef Vector<StringVector> StringMatrix;    // Note, using non standard Vector class

// Struct for string info
struct StringInfo{
    int8_t text_size;
    uint16_t width;
    uint16_t height;
};      

// -------------------------------------- SIMPLE GFX WRAPPER CLASS --------------------------------------

// Simple wrapper class for the Adafruit_TFTLCD class and individual SPI drivers from Adafruit
// Set the template typename for generic TFT class type. This means any TFT class can be used, as long as it has the same functions as the Adafruit_TFTLCD class
template <typename TFTClass>
class Simple_GFX {
public:
    /*  SUPPORTED OBJECTS:

        Adafruit_TFTLCD
        Adafruit_HX8357
        Adafruit_ST7735
        Adafruit_ILI9341
        Adafruit_SSD1351
        Adafruit_ST7789
        Adafruit_RA8875
        Adafruit_HX8357

    Initialize in your main.cpp file, and send a pointer to the Simple_GFX constructor to then send to the page classes
    example:

    TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
    Adafruit_TFTLCD tftlcd(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
    Simple_GFX<Adafruit_TFTLCD> tft(&tftlcd);
    MainPage main_page(&tft, &ts);           // MainPage is a Page object

    or 

    TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
    Adafruit_HX8357 spitft = Adafruit_HX8357(LCD_CS, LCD_DC, LCD_RST);
    Simple_GFX<decltype(spitft)> tft(&spitft);      // Type of TFTClass is Adafruit_HX8357, which is set at compile time
    MainPage main_page(&tft, &ts);                  // MainPage is a Page object
    */

    // Default constructor
    Simple_GFX() = default;     
    Simple_GFX(TFTClass *tft, int rotation) : _tft(tft), _rotation(rotation) {};
    Simple_GFX(TFTClass *tft, int rotation, uint16_t width, uint16_t height) : _tft(tft), _rotation(rotation), _width(width), _height(height) {};

    // BEGIN METHODS FOR EITHER TFTLCD or DERIVED SPITFT CLASSES
    void begin_8bit(uint16_t id = 0x9325) { _tft->begin(id); };     // For TFTLCD object
    void begin_spi(uint32_t freq = 0) { _tft->begin(freq); };       // For SPITFT objects

    void init(){
        // TFT setup
        #ifdef USE_TFTLCD	// TFT 8bit setup
            _tft->reset();
            uint16_t identifier = _tft->readID();
            begin_8bit(identifier);
        #else	// TFT SPI setup
            begin_spi();
        #endif
        delay(100);        // Wait for the screen to initialize
        _tft->setRotation(_rotation);

        // If the width and height are not set, set them to the default values
        if (_width == 0 || _height == 0){
            _width = _tft->width();
            _height = _tft->height();
        }
    };

    // ----------------------- Methods from the Adafruit_TFTLCD/Adafruit_SPITFT/Adafrui_GFX classes in context of Simple_GFX -----------------------
    // NOT ALL METHODS ARE IMPLEMENTED, if you need a method then you can use the getter function

    // Shape and Line Drawing methods
    void drawPixel(int16_t x, int16_t y, uint16_t color) { _tft->drawPixel(x, y, color); };
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) { _tft->drawLine(x0, y0, x1, y1, color); };
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { _tft->drawRect(x, y, w, h, color); };
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) { _tft->drawCircle(x0, y0, r, color); };
    void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color) { _tft->drawCircleHelper(x0, y0, r, cornername, color); };
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) { _tft->fillCircle(x0, y0, r, color); };
    void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, int16_t delta, uint16_t color) { _tft->fillCircleHelper(x0, y0, r, cornername, delta, color); };
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) { _tft->drawTriangle(x0, y0, x1, y1, x2, y2, color); };
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) { _tft->fillTriangle(x0, y0, x1, y1, x2, y2, color); };
    void drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color) { _tft->drawRoundRect(x0, y0, w, h, radius, color); };
    void fillRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color) { _tft->fillRoundRect(x0, y0, w, h, radius, color); };
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) { _tft->drawFastHLine(x, y, w, color); };
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) { _tft->drawFastVLine(x, y, h, color); };
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) { _tft->fillRect(x, y, w, h, color); };
    void fillScreen(uint16_t color) { _tft->fillScreen(color); };

    // Bitmap methods
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) { _tft->drawBitmap(x, y, bitmap, w, h, color); };
    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color, uint16_t bg) { _tft->drawBitmap(x, y, bitmap, w, h, color, bg); };
    void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color) { _tft->drawBitmap(x, y, bitmap, w, h, color); };
    void drawBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, uint16_t bg) { _tft->drawBitmap(x, y, bitmap, w, h, color, bg); };
    void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) { _tft->drawXBitmap(x, y, bitmap, w, h, color); };
    void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h) { _tft->drawGrayscaleBitmap(x, y, bitmap, w, h); };
    void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, int16_t w, int16_t h) { _tft->drawGrayscaleBitmap(x, y, bitmap, w, h); };
    void drawGrayscaleBitmap(int16_t x, int16_t y, const uint8_t bitmap[], const uint8_t mask[], int16_t w, int16_t h) { _tft->drawGrayscaleBitmap(x, y, bitmap, mask, w, h); };
    void drawGrayscaleBitmap(int16_t x, int16_t y, uint8_t *bitmap, uint8_t *mask, int16_t w, int16_t h) { _tft->drawGrayscaleBitmap(x, y, bitmap, mask, w, h); };
    void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], int16_t w, int16_t h) { _tft->drawRGBBitmap(x, y, bitmap, w, h); };
    void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h) { _tft->drawRGBBitmap(x, y, bitmap, w, h); };
    void drawRGBBitmap(int16_t x, int16_t y, const uint16_t bitmap[], const uint8_t mask[], int16_t w, int16_t h) { _tft->drawRGBBitmap(x, y, bitmap, mask, w, h); };
    void drawRGBBitmap(int16_t x, int16_t y, uint16_t *bitmap, uint8_t *mask, int16_t w, int16_t h) { _tft->drawRGBBitmap(x, y, bitmap, mask, w, h); };
        
    // Screen methods
    #ifdef USE_TFTLCD
        void reset(void) { _tft->reset(); };        // Only available for LCDTFC
    #endif
    void setRegisters8(uint8_t *ptr, uint8_t n) { _tft->setRegisters8(ptr, n); };
    void setRegisters16(uint16_t *ptr, uint8_t n) { _tft->setRegisters16(ptr, n); };
    void setRotation(uint8_t r) { _tft->setRotation(r); };
    void invertDisplay(bool i) { _tft->invertDisplay(i); };

    // Screen getter methods
    uint8_t getRotation(void) { return _tft->getRotation(); };
    int16_t getCursorX(void) { return _tft->getCursorX(); };
    int16_t getCursorY(void) { return _tft->getCursorY(); };

    // Text methods
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size) { _tft->drawChar(x, y, c, color, bg, size); };
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) { _tft->drawChar(x, y, c, color, bg, size_x, size_y); };
    void getTextBounds(const char *string, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) { _tft->getTextBounds(string, x, y, x1, y1, w, h); };
    void getTextBounds(const __FlashStringHelper *s, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) { _tft->getTextBounds(s, x, y, x1, y1, w, h); };
    void getTextBounds(const String &str, int16_t x, int16_t y, int16_t *x1, int16_t *y1, uint16_t *w, uint16_t *h) { _tft->getTextBounds(str, x, y, x1, y1, w, h); };
    void setTextSize(uint8_t s) { _tft->setTextSize(s); };
    void setTextSize(uint8_t sx, uint8_t sy) { _tft->setTextSize(sx, sy); };
    void setFont(const GFXfont *f = NULL) { _tft->setFont(f); };
    void setCursor(int16_t x, int16_t y) { _tft->setCursor(x, y); };
    void setTextColor(uint16_t c) { _tft->setTextColor(c); };
    void setTextColor(uint16_t c, uint16_t bg) { _tft->setTextColor(c, bg); };
    void setTextWrap(bool w) { _tft->setTextWrap(w); };


    // ----------------------- Methods from the Print.h in context of Simple_GFX -----------------------
    // Note: DEC is defined in Print.h as 10
    size_t println(const __FlashStringHelper *f) { return _tft->println(f); };
    size_t println(const String &s) { return _tft->println(s); };
    size_t println(const char c[]) { return _tft->println(c); };
    size_t println(char c) { return _tft->println(c); };
    size_t println(unsigned char b, int decimals = DEC) { return _tft->println(b, decimals); };
    size_t println(int n, int decimals = DEC) { return _tft->println(n, decimals); };
    size_t println(unsigned int n, int decimals = DEC) { return _tft->println(n, decimals); };
    size_t println(long n, int decimals = DEC) { return _tft->println(n, decimals); };
    size_t println(unsigned long n, int decimals = DEC) { return _tft->println(n, decimals); };
    size_t println(double n, int decimals = 2) { return _tft->println(n, decimals); };
    size_t println(const Printable &x) { return _tft->println(x); };
    size_t println(void) { return _tft->println(); };

    size_t print(const __FlashStringHelper *f) { return _tft->print(f); };
    size_t print(const String &s) { return _tft->print(s); };
    size_t print(const char c[]) { return _tft->print(c); };
    size_t print(char c) { return _tft->print(c); };
    size_t print(unsigned char b, int decimals = DEC) { return _tft->print(b, decimals); };
    size_t print(int n, int decimals = DEC) { return _tft->print(n, decimals); };
    size_t print(unsigned int n, int decimals = DEC) { return _tft->print(n, decimals); };
    size_t print(long n, int decimals = DEC) { return _tft->print(n, decimals); };
    size_t print(unsigned long n, int decimals = DEC) { return _tft->print(n, decimals); };
    size_t print(double n, int decimals = 2) { return _tft->print(n, decimals); };
    size_t print(const Printable &x) { return _tft->print(x); };

    int getWriteError() { return _tft->getWriteError(); };
    void clearWriteError() { _tft->clearWriteError(); };
    int availableForWrite() { return _tft->availableForWrite(); };
    size_t write(const char *str) { return _tft->write(str); };
    size_t write(const char *buffer, size_t size) { return _tft->write(buffer, size); };


    // ---------------------------------- Custom methods ----------------------------------
    // Get the TFT object, can be used to call any method from the Adafruit_TFTLCD/Adafruit_SPITFT class that is not implemented in the Simple_GFX class
    TFTClass* get(){
        /*
        example of usage:
        simple_gfx_object.get()->drawPixel(0, 0, BLACK);
        */
        return _tft;
    }

    uintptr_t get_tft_address_as_int(){
        // can print to Serial using: Serial.println(sim_gfx.get_tft_address_as_int(), HEX);
        return (uintptr_t)_tft;     
    }

    bool check_tft_pointer_exists(){
        // Check if the pointer to the TFT object exists
        if(!_tft){
            return false;
        }
        return true;
    }


    // Return the width and height of the TFT display, forward overrides of the Adafruit_GFX
    int16_t width(void) { return _width; };
    int16_t height(void) { return _height; };

private:
    TFTClass *_tft;
    int _rotation;
    uint16_t _width, _height;
};

// -------------------------------------- UTILS --------------------------------------

// Utils for the menu and other classes
template<typename TFTClass>
class Utils {
public:
    Utils(){};
    
    // Gets a pointer and returns the address of the pointer as an uint, print to serial as HEX to see the address
    static uintptr_t get_pointer_address(void* ptr) {
        return reinterpret_cast<uintptr_t>(ptr);
    }

    // Convert a String to a matrix of StringVectors where each StringVector comprises of words separated by spaces
    StringMatrix text_to_matrix_of_words(String *text){
        // Convert the String to a StringVector type where words are separated by spaces and new lines are denoted as $ 
        // Example:
        // input : String text = "This is text$with multiple lines";
        // output : StringMatrix text_vector = {{"This", "is", "text"}, {"with", "multiple", "lines"}};
        //      where text_vector[0] = {"This", "is", "text"} as a StringVector type, and text_vector[1] = {"with", "multiple", "lines"} as a StringVector type 
        
        // Split the text into StringVectors where each element is a line of text
        StringVector lines = text_to_vector_of_lines(text);

        // Convert the StringVector of lines to a StringMatrix of words, where each line is a StringVector of words    
        return vector_of_lines_to_matrix_of_words(lines);
    };      

    // Convert a String to a StringVector where each String is a new line
    StringVector text_to_vector_of_lines(String *text){
        // Iterate through all of the characters in the string, and split the String at '$' characters
        // Example:
        // String text = "This is text$with multiple lines";
        // StringVector lines = {"This is text", "with multiple lines"};
        StringVector lines;
        int16_t text_index = 0;
        int16_t text_length = text->length();
        for (int i=0; i<=text_length; i++){
            if (text->charAt(i)=='$' || i==text_length){
                // Append string to vector
                String sub = text->substring(text_index, i);
                lines.push_back(sub);
                // Get start index for next search
                text_index = i+1;
            }
        }
        return lines;
    };

    // Convert a String to a StringVector where each element is a word separated by spaces
    StringVector text_to_vector_of_words(String *text){
        // Iterate through all of the characters in the string, and split the String at ' ' characters
        // Example:
        // String text = "This is text";
        // StringVector words = {"This", "is", "text"};
        StringVector words;
        int16_t text_index = 0;
        int16_t text_length = text->length();
        for (int i=0; i<=text_length; i++){
            if (text->charAt(i)==' ' || i==text_length){
                // Append string to vector
                String sub = text->substring(text_index, i);
                words.push_back(sub);
                // Get start index for next search
                text_index = i+1;
            }
        }
        return words;
    };

    // Convert a StringVector of lines to a StringMatrix of words, where each line is a StringVector of words
    StringMatrix vector_of_lines_to_matrix_of_words(StringVector lines){
        // Iterate through the lines, and split the String at ' ' characters. Store the words in a StringVector, and then store each line in a StringMatrix
        // Example:
        // StringVector lines = {"This is text", "with multiple lines"};
        // StringMatrix text_vector = {{"This", "is", "text"}, {"with", "multiple", "lines"}};
        StringVector words;
        StringMatrix text_matrix;
        String sub, line;
        int8_t num_lines = lines.size();

        // Go line by line and append the words to the words StringVector, and then the words StringVector to the StringMatrix
        for (int16_t i=0; i<num_lines; i++){
            line = lines[i];                                        // Extract the line from the StringVector as a String
            words = text_to_vector_of_words(&line);                 // Convert the line to a StringVector of words
            // Append the words to the StringMatrix
            text_matrix.push_back(words);
            // Clear the words StringVector for the next line
            words.clear();
        }
        return text_matrix;
    };

    // Get info about a string given the text, text size, and font
    StringInfo get_text_info(Simple_GFX<TFTClass> *sim_gfx, String text, int8_t text_size, const GFXfont *font){
        // Get the width and height of the text
        int16_t _x1, _y1;
        uint16_t _w1, _h1;
        sim_gfx->setFont(font);
        sim_gfx->setTextSize(uint8_t(text_size));
        sim_gfx->getTextBounds(text, 0, 0, &_x1, &_y1, &_w1, &_h1);
        
        // Return the text info
        StringInfo text_info;
        text_info.text_size = text_size;
        text_info.width = _w1;
        text_info.height = _h1;
        return text_info;
    };

    String vector_of_words_to_string(StringVector *words){
        // Construct a String from a given StringVector, where each word in the vector is separated by a space
        String text = "";
        int16_t num_words = words->size();
        for (int16_t i=0; i<num_words; i++){
            // Append the word to the text
            text += words->at(i);
            // If not last word, then add a space
            if (i != num_words-1){
                text += " ";
            }
        }
        return text;
    };    

    // Contruct a StringVector from a given String for a text box where each element in a StringVector is a line of text that fits the width of the text box
    StringVector text_to_textbox_string_vector(Simple_GFX<TFTClass> *sim_gfx, String *text, uint16_t text_size, uint16_t box_width, const GFXfont *font){
        // Can pass text that has '$' characters to denote new lines, when the new line character is reached, a new line will be started
        // Example:
        // String text = "This is text$with multiple lines. This is the more text that is long enough to wrap to the next line.";
        // StringVector textbox_lines = {"This is text", "with multiple lines. This is the more text", "that is long enough to wrap to the next", "line."};
        // Each line must fit in the textbox width

        // Get the text as a StringVector where the lines are split at the '$' character. If there is no '$' character, then the StringVector will only have one element
        StringVector split_lines = text_to_vector_of_lines(text);

        // Iterate through each line and create the longes String that still fits in the width of the textbox. Store each line in a StringVector
        StringVector lines;     // Vector of lines that will be returned by the function
        String line;
        StringVector words;
        String word;
        int16_t num_words;
        uint16_t text_width, width;
        int8_t num_lines = split_lines.size();
        for (int8_t i=0; i < num_lines; i++){
            line = split_lines[i];                          // Get the line as a String
            words = text_to_vector_of_words(&line);         // Get the words in the line as a StringVector
            num_words = words.size();                       // Get the number of words in the line
            text_width = 0;                                 // Reset the text width
            line = "";                                      // Reset the line String
            // Iterate through each word in the line and add it to the line String until the width of the line is greater than the width of the textbox
            for (int8_t j=0; j<num_words; j++){
                word = words[j];                            // Get the word as a String
                width = get_text_info(sim_gfx, word, text_size, font).width;    // Get the width of the word
                text_width += width;                        // Add the width to the text width
                // If the width of the line is greater than the width of the textbox, then add the line to the lines StringVector and start a new line
                if (text_width > width){
                    lines.push_back(line);
                    line = "";
                    text_width = 0;
                }
                // Add the word to the line
                line += word;
                // Add a space to the line if it is not the last word in the line
                if (j != num_words-1){
                    line += " ";
                }
            }
            // Add the last line to the lines StringVector
            lines.push_back(line);
        }
        return lines;
    };    

    // Print a String to the screen given the text, text size, font, x and y coordinates of the TOP LEFT corner of the bounding box wanted
    // This is used to print text with custom font as the default fonts would print
    void print_text_tl_coords(Simple_GFX<TFTClass> *sim_gfx, int16_t x_tl, int16_t y_tl, String text, uint16_t text_color, int8_t text_size, const GFXfont *font, uint16_t text_bg_color=-1){
        // If the font is NULL, then use the default font and use the x and y given to print the String
        if (font == NULL){
            // If the font background color is not NULL then set the background color and face color
            if (text_bg_color != -1){
                sim_gfx->setTextColor(text_color, text_bg_color);
            } else {
                sim_gfx->setTextColor(text_color);
            }
            sim_gfx->setFont();
            sim_gfx->setTextSize(uint8_t(text_size));
            sim_gfx->setCursor(x_tl, y_tl);
            sim_gfx->println(text);
        } else {
            // Using custom font, so need to get the text info (height) to offset the y coordinate
            StringInfo text_info = get_text_info(sim_gfx, text, text_size, font);
            // Set the font and text size
            sim_gfx->setFont(font);
            sim_gfx->setTextSize(uint8_t(text_size));
            sim_gfx->setTextColor(text_color);                          // Note, custom fonts cannot use background colors
            sim_gfx->setCursor(x_tl, y_tl + text_info.height);
            sim_gfx->println(text);
        }
    };

    ~Utils(){};    

    // Alignment of the text in bounding boxes
    typedef enum {
        RIGHT,
        CENTER, 
        LEFT
    } TextAlignment;  

    String DUMMY_STR = "DUMMY";   // Dummy string to use for getting string height 
};

// -------------------------------------- TITLE --------------------------------------

// Create a title object, can be used to display a title on a page
template<typename TFTClass>
class Title : public Utils<TFTClass> {
public:
    
    Title(Simple_GFX<TFTClass> *sim_gfx) : _sim_gfx(sim_gfx){
        font = &FreeSans24pt7b;
    };

    // Initialize the title parameters
    void init_title(uint16_t text_color, int8_t text_size, typename Utils<TFTClass>::TextAlignment alignment, String *text){
        // Set title properties
        _text_color = text_color;
        _text_size = text_size;
        _alignment = alignment;
        _text = text;
    };

    // Draw the title to the screen
    void draw(void){
        // Get the text for the title as a Vector of lines
        StringVector lines = text_to_textbox_string_vector(_sim_gfx, _text, _text_size, _sim_gfx->width(), font);
        int16_t text_height = get_text_info(_sim_gfx, Utils<TFTClass>::DUMMY_STR, _text_size, font).height;  
        
        _h = text_height * lines.size() + (lines.size()-1) * text_line_gap;
        _w = _sim_gfx->width() - 2*side_gap;
        _x = side_gap;
        _y = top_gap;
        bottom_title_y = _y + _h;

        // For each line in the title, draw the text to the screen, and make sure it is printed to match to the alignment set
        int16_t num_lines = lines.size();
        for (int16_t i=0; i<num_lines; i++){
            // Get the line
            String line = lines.at(i);
            // Get the width of the line
            int16_t line_width = get_text_info(_sim_gfx, line, _text_size, font).width;
            // Get the x coordinate of the line based on the alignment
            int16_t line_x, line_y;
            switch (_alignment) {
                case Utils<TFTClass>::TextAlignment::CENTER:
                    line_x = _x + (_w/2) - (line_width/2);
                    line_y = _y + (_h/2) - (text_height/2) - ((text_height+text_line_gap) * (num_lines-1)/2) + ((text_height+text_line_gap) * (i-1));
                    break;
                case Utils<TFTClass>::TextAlignment::LEFT:
                    line_x = _x;
                    line_y = _y + ((text_height+text_line_gap) * (i-1));
                    break;
                case Utils<TFTClass>::TextAlignment::RIGHT:
                    line_x = _x + _w - line_width;
                    line_y = _y + ((text_height+text_line_gap) * (i-1));
                    break;
            }
            // Print the line to the screen
            print_text_tl_coords(_sim_gfx, line_x, line_y, line, _text_color, _text_size, font);
        }
    };
    ~Title(){};

    int8_t top_gap = 3;                 // Pixels to leave between top of screen and top of title letters
    int8_t text_line_gap = 3;           // Pixels to leave between each lines of text
    int8_t side_gap = 5;                // Pixels to leave between side of screen and side of title letters, this is only used if not CENTER alignment

    int16_t get_bottom_y(void){return bottom_title_y;};         // Returns the bottom_title_y variable

private:
    Simple_GFX<TFTClass> *_sim_gfx;
    const GFXfont *font;  
    int16_t _x, _y;                     // X and Y coordinate of top left corner of title boudning box
    uint16_t _w, _h;                    // Width and Height of title bounding box
    typename Utils<TFTClass>::TextAlignment _alignment;         // Alignment of the text in the title
    uint16_t _text_color;               // Color of title text
    int8_t _text_size;                  // Size of title text
    String *_text;                      // Text to display in title
    int8_t bottom_title_y;              // Y coordinate of bottom of title bounding box
};


// -------------------------------------- TEXTBOX --------------------------------------

// Creates a textbox object, can be used to display text on a page
template<typename TFTClass>
class Textbox : public Utils<TFTClass> {
public:

    Textbox(Simple_GFX<TFTClass> *sim_gfx) : _sim_gfx(sim_gfx){
        font = &FreeSans9pt7b;
    };
    void init_textbox_center_coords(int8_t x, int8_t y, int8_t width, int8_t height, 
                                    uint16_t fill_color, uint16_t border_color, int8_t border_width, int8_t corner_radius, int8_t border_padding,
                                    uint16_t text_color, int8_t text_size, typename Utils<TFTClass>::TextAlignment alignment, String *text){
        // Set textbox properties
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
        _alignment = alignment;
        _text = text;
    };

    void init_textbox_top_left_coords(int8_t x_top_left, int8_t y_top_left, int8_t width, int8_t height, 
                                      uint16_t fill_color, uint16_t border_color, int8_t border_width, int8_t corner_radius, int8_t border_padding,
                                      uint16_t text_color, int8_t text_size, typename Utils<TFTClass>::TextAlignment alignment, String *text){

        // Set textbox properties
        _x = x_top_left;  
        _y = y_top_left;  
        _w = width;
        _h = height;
        _fill_color = fill_color;
        _border_color = border_color;
        _border_width = border_width;
        _corner_radius = corner_radius;
        _border_padding = border_padding;
        _text_color = text_color;
        _text_size = text_size;
        _alignment = alignment;
        _text = text;
    };
    void update_text(String *new_text);

    // Draws the button to the screen
    void draw(void){
        // Draw the textbox shape
        if (_border_width > 1) {
            // If border width is greater than one then draw rounded rectangle with fill color and then no fill border rectangle
            _sim_gfx->fillRoundRect(_x, _y, _w, _h, _corner_radius, _border_color);             // Draw border first
            _sim_gfx->fillRoundRect(_x+_border_width, _y+_border_width, _w-_border_width*2, 
                                _h-_border_width*2, _corner_radius, _fill_color);           // Draw fill second
        } else if (_border_width == 1) {
            // If border width is one then draw rounded rectangle with fill color and then no fill border rectangle
            _sim_gfx->fillRoundRect(_x, _y, _w, _h, _corner_radius, _fill_color);       // Draw fill first
            _sim_gfx->drawRoundRect(_x, _y, _w, _h, _corner_radius, _border_color);     // Draw border second
        } else {
            // If border width is zero then draw rounded rectangle with fill color and no border rectangle
            _sim_gfx->fillRoundRect(_x, _y, _w, _h, _corner_radius, _fill_color);
        }

        // Get the text for the textbox as a vector of lines
        StringVector lines = text_to_textbox_string_vector(_sim_gfx, _text, _text_size, _w, font); 
        int16_t text_height = get_text_info(_sim_gfx, Utils<TFTClass>::DUMMY_STR, _text_size, font).height;   

        // For each line in the textbox, draw the text to the screen, and make sure it is printed to match to the alignment set
        int16_t num_lines = lines.size();
        for (int16_t i=0; i<num_lines; i++){
            // Get the line
            String line = lines.at(i);
            // Get the width of the line
            int16_t line_width = get_text_info(_sim_gfx, line, _text_size, font).width;
            // Get the x coordinate of the line based on the alignment
            int16_t line_x, line_y;
            switch (_alignment) {
                case Utils<TFTClass>::TextAlignment::CENTER:
                    line_x = _x + (_w/2) - (line_width/2);
                    line_y = _y + (_h/2) - (text_height/2) - (text_height * (num_lines-1)/2) + ((text_height+text_line_gap) * i);
                    break;
                case Utils<TFTClass>::TextAlignment::LEFT:
                    line_x = _x + _border_padding + _border_width + text_side_padding;
                    line_y = _y + _border_padding + _border_width + text_top_padding + ((text_height+text_line_gap) * i);
                    break;
                case Utils<TFTClass>::TextAlignment::RIGHT:
                    line_x = _x + _w - _border_padding -_border_width - text_side_padding - line_width;
                    line_y = _y + _border_padding + _border_width + text_top_padding + ((text_height+text_line_gap) * i);
                    break;
            }
            // Print the line to the screen
            print_text_tl_coords(_sim_gfx, line_x, line_y, line, _text_size, _text_color, font);
        }
    };
    ~Textbox(){};

    
private:
    Simple_GFX<TFTClass> *_sim_gfx;                 // Pointer to the tft object
    const GFXfont *font;

    int16_t _x, _y;                             // X and Y coordinate of top left corner
    int16_t _w, _h;                             // Height and Width of textbox rectangle
    uint16_t _fill_color;                       // Color of textbox rectangle
    uint16_t _border_color;                     // Color of textbox border
    int8_t _border_width;                       // Width of textbox border
    int8_t _corner_radius;                      // Radius of textbox corners
    int8_t _border_padding;                     // Padding between drawn border and the bounding box
    typename Utils<TFTClass>::TextAlignment _alignment;       // Alignment of the text in the textbox
    uint8_t _text_color;                        // Color of textbox text
    int8_t _text_size;                          // Size of textbox text
    String *_text;                              // Text to display in textbox
    String _new_text;                           // New text to display in textbox

    int8_t text_side_padding = 3;               // Pixels to leave between side of DRAWN textbox and side of text
    int8_t text_top_padding = 3;                // Pixels to leave between top of DRAWN textbox and top of text
    int8_t text_line_gap = 3;                   // Pixels to leave between each lines of text

    
};

// -------------------------------------- BUTTON --------------------------------------

// struct ButtonData {
//     int8_t goto_page_number;
//     bool button_state;
//     float *dummy_float_data[6];      // Can be used to store float data pointers for the button to be sent to callback function
//     uint32_t *dummy_int_data[6];     // Can be used to store uint32_t data pointers for the button to be sent to callback function
//     String *dummy_string_data[6];    // Can be used to store String data pointers for the button to be sent to callback function
// };

// Creates a button object, can be used to create a button on a page
template<typename TFTClass>
class Button : public Utils<TFTClass> {
public:

    Button() = default;
    Button(Simple_GFX<TFTClass> *sim_gfx, TouchScreen *ts) : function_callback(nullptr), _sim_gfx(sim_gfx), _ts(ts){
        font = &FreeSansBold12pt7b;
    };
    
    // Type of button, toggle or momentary
    typedef enum {
        TOGGLE,
        MOMENTARY
    } ButtonType;           

    // Returns the _sim_gfx object address
    uintptr_t get_sim_gfx_address(){
        return (uintptr_t)&_sim_gfx;
    }

    // Initialize the button parameters given the XY coordinates of the center of the button bounding box
    void init_button_center_coords(int16_t x, int16_t y, int16_t width, int16_t height, 
                                         uint16_t fill_color, uint16_t border_color, uint16_t press_fill_color, uint16_t press_border_color,
                                         int16_t border_width, int16_t corner_radius, int16_t border_padding,
                                         uint16_t text_color, int16_t text_size, String *label, ButtonType type){
        // Set button properties
        _x = x - (width/2);           // convert to top left coord
        _y = y - (height/2);          // convert to top left coord
        _w = width;
        _h = height;
        _fill_color = fill_color;
        _border_color = border_color;
        _press_fill_color = press_fill_color;
        _press_border_color = press_border_color;
        _border_width = border_width;
        _corner_radius = corner_radius;
        _border_padding = border_padding;
        _text_color = text_color;
        _text_size = text_size;
        _label = Utils<TFTClass>::text_to_vector_of_lines(label);
        //_input_label = &label;
        _type = type;                                         
    };

    // Initialize the button parameters given the XY coordinates as the top left corner of the button bounding box
    void init_button_top_left_coords(int16_t x_top_left, int16_t y_top_left, int16_t width, int16_t height, 
                                         uint16_t fill_color, uint16_t border_color, uint16_t press_fill_color, uint16_t press_border_color,
                                         int16_t border_width, int16_t corner_radius, int16_t border_padding,
                                         uint16_t text_color, int16_t text_size, String *label, ButtonType type){
        // Set button properties
        _x = x_top_left;
        _y = y_top_left;
        _w = width;
        _h = height;
        _fill_color = fill_color;
        _border_color = border_color;
        _press_fill_color = press_fill_color;
        _press_border_color = press_border_color;
        _border_width = border_width;
        _corner_radius = corner_radius;
        _border_padding = border_padding;
        _text_color = text_color;
        _text_size = text_size;
        _label = Utils<TFTClass>::text_to_vector_of_lines(label);
        //_input_label = &label;
        _type = type;
    };

    // Set the min and max pressure for the touch screen to register a touch
    void set_min_max_pressure(int16_t min_pressure, int16_t max_pressure){
        _min_pressure = min_pressure;
        _max_pressure = max_pressure;
    };

    // Set the min and max XY values of the screen to register a touch
    void set_min_max_x_y_values(int16_t min_x, int16_t max_x, int16_t min_y, int16_t max_y){
        _min_x = min_x;
        _max_x = max_x;
        _min_y = min_y;
        _max_y = max_y;
    };

    // Draw the button to the screen. If the button is pressed, use the press colors, else use the normal colors
    void draw(bool pressed=false){
        // ----------------------------- Draw button shape -----------------------------
        // Draw button rectangle, then border over it
        int16_t x = _x + _border_padding;
        int16_t y = _y + _border_padding;
        int16_t w = _w - _border_padding*2;
        int16_t h = _h - _border_padding*2;
        int16_t r = _corner_radius;
        uint16_t b_color, f_color;

        // Check to see if the tft pointer is valid
        // Print the address of _sim_gfx
        //uintptr_t addr = Utils<TFTClass>::get_pointer_address(_sim_gfx);
        //Serial.print("Button sim_gfx address: "); Serial.println(addr, HEX);
        
        // Set the colors depending on if the button is pressed/toggled or not
        if (pressed){         
            f_color = _press_fill_color;
            b_color = _press_border_color;
        } else {
            f_color = _fill_color;
            b_color = _border_color;
        }        
        
        // Draw button rectangle and border
        if (_border_width > 1) {
            // If border width is greater than one then draw rounded rectangle with fill color and then no fill border rectangle
            _sim_gfx->fillRoundRect(x, y, w, h, r, b_color);        // Draw border first
            _sim_gfx->fillRoundRect(x+_border_width, y+_border_width, w-_border_width*2, h-_border_width*2, r, f_color);       // Draw fill first
        } else if (_border_width == 1) {
            // If border width is one then draw rounded rectangle with fill color and then no fill border rectangle
            _sim_gfx->fillRoundRect(x, y, w, h, r, f_color);        // Draw fill first
            _sim_gfx->drawRoundRect(x, y, w, h, r, b_color);        // Draw border second
        } else {
            // If border width is zero then draw rounded rectangle with fill color and no border rectangle
            _sim_gfx->fillRoundRect(x, y, w, h, r, f_color);
        }
        
        
        // --------------------------- Draw button text ---------------------------
        // Iterate through the StringVector and print each string on a new line
        int16_t x1, y1;
        uint16_t w1, h1;
        int16_t x_tl, y_tl;
        int8_t num_lines = _label.size();

        Serial.print(F("Num lines in label: ")); Serial.println(num_lines);
        Serial.print(F("Printing button label: ")); Serial.println(_input_label);
        
        // Calculate the vertical offset for the text due to multiple lines
        StringInfo temp_info = Utils<TFTClass>::get_text_info(_sim_gfx, Utils<TFTClass>::DUMMY_STR, _text_size, font);
        int16_t vertical_offset = (temp_info.height * num_lines) / 2 + (text_line_gap * (num_lines-1));      // divide by 2 to get half height, then add line gap for each line

        for (int8_t i = 0; i < num_lines; i++) {
            // Get the line string
            String line_string = _label[i];   
            // Get the width and height of the text
            _sim_gfx->getTextBounds(line_string, 0, 0, &x1, &y1, &w1, &h1);
            // Print the text
            x_tl = x + (w/2) - (w1/2);
            y_tl = y + (h/2) - (h1/2) - ((num_lines-i)/2 * vertical_offset);
            Utils<TFTClass>::print_text_tl_coords(_sim_gfx, x_tl, y_tl, line_string, _text_size, _text_color, font);
        }
    };

    // Used to update the button give touch input, called every loop.
    void update(){
        // See of the button is pressed
        curr_touch = is_pressed();

        // While button is being pressed, change the colors of the button. For Toggle, this is done when the button is pressed and released
        if (curr_touch && !last_touch) {
            if (_type == TOGGLE) {
                // Do not do anything if the button is toggle button
            } else {    
                // If the button is not a toggle button (is momentary), then change the colors for pressed state
                draw(true);
                _pressed = true;
            }
        }

        // If button is pressed and then released
        if (!curr_touch && last_touch) {
            if (_type == TOGGLE) {
                // Update the button colors if the button is a toggle button
                _pressed = !_pressed;       // Reverse the press to toggle/untoggle button
                draw(true);
            } else {
                // If the button is not a toggle button (is momentary), then return colors to normal
                _pressed = false;
                draw(false);
            }
            call_callback();     // Call the callback function
        }
        last_touch = curr_touch;
    };       

    // Callback function with no arguments
    void attach_callback_function(void (*function)(void)){
        this->function_callback = function;
    };

    // Get the button's label
    String get_label(void){
        return _input_label;
    };

    // Check to see if the SCREEN is pressed and return true if it is
    bool is_pressed(void){
        // Get the current touch point
        TSPoint touch_point = _ts->getPoint();
        // If the touch point is valid
        if (touch_point.z > _min_pressure && touch_point.z < _max_pressure) {
            // If rotation is 0, then x and y are swapped
            _t_x = (uint16_t)map(touch_point.y, _min_x, _max_x, 0, _sim_gfx->width());     
            _t_y = (uint16_t)map(touch_point.x, _min_y, _max_y, 0, _sim_gfx->height());

            // Check if the touch point is within the button bounds
            if (touch_point.x > _x && touch_point.x < _x + _w && touch_point.y > _y && touch_point.y < _y + _h) {
                // If so, then return true
                return true;
            }
        }
        // Otherwise, return false
        return false;
    };

    // // Print the label of the button on the button shape
    // void print_label(void){
    //     // Get DRAWN button dimensions
    //     uint16_t x = _x + _border_padding;
    //     uint16_t y = _y + _border_padding;
    //     uint16_t w = _w - _border_padding*2;
    //     uint16_t h = _h - _border_padding*2;

    //     // Iterate through the StringVector and print each string on a new line
    //     int16_t x1, y1;
    //     uint16_t w1, h1;
    //     int16_t x_tl, y_tl;
    //     int8_t num_lines = _label.size();

    //     Serial.print(F("Num lines in label: ")); Serial.println(num_lines);
    //     Serial.print(F("Printing button label: ")); Serial.println(_input_label);
        
    //     // Calculate the vertical offset for the text due to multiple lines
    //     StringInfo temp_info = Utils<TFTClass>::get_text_info(_sim_gfx, Utils<TFTClass>::DUMMY_STR, _text_size, font);
    //     int16_t vertical_offset = (temp_info.height * num_lines) / 2 + (text_line_gap * (num_lines-1));      // divide by 2 to get half height, then add line gap for each line

    //     for (int8_t i = 0; i < num_lines; i++) {
    //         // Get the line string
    //         String line_string = _label[i];   
    //         // Get the width and height of the text
    //         _sim_gfx->getTextBounds(line_string, 0, 0, &x1, &y1, &w1, &h1);
    //         // Print the text
    //         x_tl = x + (w/2) - (w1/2);
    //         y_tl = y + (h/2) - (h1/2) - ((num_lines-i)/2 * vertical_offset);
    //         Utils<TFTClass>::print_text_tl_coords(_sim_gfx, x_tl, y_tl, line_string, _text_size, _text_color, font);
    //     }
    // };

    virtual ~Button(){};

    // Set vars, can override if needed
    int8_t text_line_gap = 3;               // Pixels to leave between each lines of text
    bool _pressed = false;                  // State for if button is pressed/toggled

protected:
    
    void (*function_callback)(void);       // Callback function for when button is pressed, takes no arguments

    // Used to call the callback function when button is pressed
    virtual void call_callback(){
        if (function_callback != nullptr){
            function_callback();
        }
    };           

private:
    Simple_GFX<TFTClass> *_sim_gfx;     // Pointer to the tft object
    TouchScreen *_ts;               // Pointer to the TouchScreen object
    const GFXfont *font;

    StringVector _label;            // Label for button as a StringVector type, where each line is printed on a new line
    String _input_label;       // Label for button as a String type, takes the input text and stores it
    int16_t _x, _y;                 // X and Y coordinate of top left corner
    int16_t _w, _h;                 // Height and Width of button rectangle
    uint16_t _fill_color;           // Color of button fill
    uint16_t _text_color;           // Color of button text
    uint16_t _border_color;         // Color of button border
    uint16_t _press_fill_color;     // Color of button fill when pressed
    uint16_t _press_border_color;   // Color of button border when pressed
    int16_t _text_size;             // Size of button text
    int16_t _corner_radius;         // Rounded corners of button rectangle
    int16_t _border_padding;        // Number of pixels to pad the button border (just for rendering, creates space around button)
    int16_t _border_width;          // Width of button border
    ButtonType _type;               // Type of button, toggle or momentary
    volatile int16_t _t_x, _t_y;    // Coordinates of the touch point
    int16_t _min_pressure;          // Minimum pressure to register as a touch
    int16_t _max_pressure;          // Maximum pressure to register as a touch
    int16_t _min_x, _max_x;         // Minimum and maximum x values to register as a touch
    int16_t _min_y, _max_y;         // Minimum and maximum y values to register as a touch
    bool curr_touch, last_touch;    // States to tell if button is pressed and released
    
};

/*
	ButtonForMethods - Class that inherits Button class and
	overwrites it's call method to support methods as
	a callback

    example with a class called ExampleClass with Button objects initiated inside the class, and public callback methods inside the class:
    
    class ExampleClass{
    public:
        ExampleClass(PageManager *page_manager, Simple_GFX *sim_gfx, TouchScreen *ts) 
                    : _manager(page_manager), _sim_gfx(tft), _ts(ts), 
                    button1(this, &ExampleClass::callback_method1),
                    button2(this, &ExampleClass::callback_method2){
            create_buttons();
        };
        void callback_method1(){
            // Do something, like:
            _manager->set_page(_manager->get_previous_page());
        };
        void callback_method2(){
            // Do something, like:
            _manager->set_page(0);
        };
        void update_buttons(){
            for (int i=0; i<2; i++){
                buttons[i]->update();
            }
        };
        void create_buttons();  
    private:
        PageManager *_manager;
        Simple_GFX *_sim_gfx;
        TouchScreen *_ts;
        Button *buttons[2];
        ButtonForMethods<ExampleClass> button1;
        ButtonForMethods<ExampleClass> button2;
    };
    
    void ExampleClass::create_buttons(){
        buttons[0] = &button1;
        buttons[1] = &button2;

        for (int i=0; i<2; i++){
            buttons[i]->init_button_center_coords(...)
            buttons[i]->set_min_max_pressure(...)
            buttons[i]->set_min_max_x_y_values(...)
        }

        button1.attach_callback_function(&ExampleClass::callback_method1);
        button2.attach_callback_function(&ExampleClass::callback_method2);
    };
*/


template <class Obj, typename TFTClass>
class ButtonForMethods: public Button<TFTClass> {
public:
	ButtonForMethods(Obj *object, Simple_GFX<TFTClass> *sim_gfx, TouchScreen *ts, void (Obj::*callback)(void)): Button<TFTClass>(sim_gfx, ts) {
		this->object = object;
        this->_sim_gfx = sim_gfx;
        this->_ts = ts;
		this->method = callback;
	}
	virtual void call() {
		return (object->*method)();
	}
    ~ButtonForMethods() override {};
private:
    Simple_GFX<TFTClass> *_sim_gfx;          // Pointer to the tft object
    TouchScreen *_ts;                    // Pointer to the TouchScreen object
	Obj *object;
	void (Obj::*method)(void);
};


// -------------------------------------- KEYBOARD --------------------------------------

// Creates an object which is a sub page that is a keyboard for user input
// Creates either a number or text keyboard
template <typename TFTClass>
class Keyboard : public Utils<TFTClass> {
public:
    Keyboard(Simple_GFX<TFTClass> *sim_gfx, TouchScreen *ts) : _sim_gfx(sim_gfx), _ts(ts){
        font = &FreeSansBold12pt7b;
    };

    void init_alphabetic_keyboard(String *target, int8_t target_char_limit, 
                                  uint16_t background_color, uint16_t bounding_box_color, uint16_t bounding_box_border_color,
                                  uint16_t button_fill_color, uint16_t button_border_color, uint16_t button_text_color);
    void init_numeric_keyboard(float *target, float min_target_value, float max_target_value, 
                               uint16_t background_color, uint16_t bounding_box_color, uint16_t bounding_box_border_color,
                               uint16_t button_fill_color, uint16_t button_border_color, uint16_t button_text_color);     // For floats as a target variable 
    void init_numeric_keyboard(int *target, int min_target_value, int max_target_value, 
                               uint16_t background_color, uint16_t bounding_box_color, uint16_t bounding_box_border_color,
                               uint16_t button_fill_color, uint16_t button_border_color, uint16_t button_text_color);       // For ints as a target variable

    
    void update();       // Used to update the keyboard give touch input, called every loop

    ~Keyboard(){};

    typedef enum {
        FLOAT,
        INTEGER,
        STRING
    } target_type;

private:
    Simple_GFX<TFTClass> *_sim_gfx;     // Pointer to the tft object
    TouchScreen *_ts;               // Pointer to the TouchScreen object
    const GFXfont *font;

    String key_input;               // String to store the keyboard input(s) temporarily
    String *_alph_target;           // Pointer to the string to save the alphabetic keyboard input(s)
    float *_float_target;           // Pointer to the float to save the numeric keyboard input(s)
    int *_int_target;               // Pointer to the int to save the numeric keyboard input(s)
    int8_t _target_char_limit;      // Character limit for the target string
    float _min_target_value;        // Minimum value for the target float AND int
    float _max_target_value;        // Maximum value for the target float AND int

    uint16_t _background_color;     // Color of the keyboard background
    uint16_t _bounding_box_color;   // Color of the fill for keyboard bounding box
    uint16_t _bounding_box_border_color;   // Color of the border for keyboard bounding box
    uint16_t _button_fill_color;    // Color of button fill
    uint16_t _button_press_fill_color = WHITE;    // Color of button fill when pressed
    uint16_t _button_text_color;    // Color of button text
    uint16_t _button_border_color;  // Color of button border
    target_type _target_type;       // Type of target variable, set in init function
    Button<TFTClass> *_buttons_numeric[13];       // Array of buttons for the numeric keyboard   
    Button<TFTClass> *_buttons_alphabetic[26];    // Array of buttons for the alphabetic keyboard
    Button<TFTClass> *_textbox;              // Textbox object for the keyboard input

    // Values calculated for the size of the keyboard objects (boundary, textbox, keys)
    int8_t num_rows, num_cols;              // Number of rows and columns of keys
    int8_t num_keys;                        // Number of keys
    int16_t total_width, total_height;     // Total width and height of keyboard
    int16_t kb_tl_x, kb_tl_y;              // Top left x and y of keyboard bounding box
    int16_t tb_w, tb_h;                    // Width and height of the text box
    int16_t tb_tl_x, tb_tl_y;              // Top left x and y of the text box
    int16_t key_width, key_height;         // Width and height of the keys

    float keyboard_width_percent = 0.5;                     // Percentage of screen width to use for keyboard, centered on screen (sets x coordinate of keyboard)
    float keyboard_height_percent = 0.5;                    // Percentage of screen height to use for keyboard (not including the input textbox)
    float key_input_text_box_height_percent = 0.1;          // Percentage of screen height to use for the input textbox
    float keyboard_bottom_spacing_percent = 0.05;           // Percentage of screen height to use for spacing between keyboard and bottom of screen (sets y coordinate of keyboard)
    uint8_t bounding_box_border_width = 1;                  // Border width for the keyboard bounding box in pixels
    uint8_t bounding_box_corner_radius = 3;                 // Corner radius for the keyboard bounding box in pixels
    uint8_t key_text_size = 2;                              // Text size for the keys 
    uint8_t key_border_width = 1;                           // Border width for the keys in pixels
    uint8_t key_corner_radius = 3;                          // Corner radius for the keys in pixels
    uint8_t object_border_padding = 2;                      // Border padding for the keys and text box in pixels
    uint16_t text_box_fill_color = WHITE;
    uint16_t text_box_border_color = BLACK;
    uint16_t text_box_text_color = BLACK;
    uint8_t text_box_text_size = 3;


    void update_key_input_string(String new_input);         // Updates the textbox that contains the input from keyboard
    void check_key_press();                                 // Checks if a key is pressed, if so then updates the key_input string
    void init_keyboard_boundary();                          // Initializes the keyboard boundary 
    void init_textbox();                                    // Initializes the textbox that contains the input from keyboard
    void init_numeric_keys();                               // Initializes the buttons for the numeric keyboard 
    void init_alphabetic_keys();                            // Initializes the buttons for the alphabetic keyboard
    void init_draw_keyboard_boundary();                     // Draws the keyboard boundary
    void init_draw_numeric_keys();                          // Draws the keys (all keys for init graphics)
    void init_draw_alphabetic_keys();                       // Draws the keys (all keys for init graphics)
    void init_draw_textbox();                               // Draws the textbox that contains the input from keyboard
};


// -------------------------------------- PAGE BASE CLASS --------------------------------------

// Create a base class for a Page
template <typename TFTClass>
class Page : public Utils<TFTClass> {
public:
    Page(){};
    virtual void initialize(void) = 0;        // Initialize the page, the buttons, and the other objects used in the page
    virtual void update(void) = 0;            // Run the individual update functions for the buttons and other objects  
    virtual ~Page(){};
};

#endif