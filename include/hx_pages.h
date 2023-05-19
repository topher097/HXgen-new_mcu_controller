#ifndef _HX_PAGES_H_
#define _HX_PAGES_H_


#pragma once
#include <Arduino.h>
#include <page.h>

#include <pages/page_nums.h>
#include <pages/main_page.h>
#include <pages/inlet_temp_page.h>
#include <pages/piezo_page.h>

class HXPages {
public:
    HXPages();
    void update();
    void add(Page *page);
    void remove(Page *page);
    void set_current_page(int8_t page_number);
    void set_previous_page(int8_t page_number);
    int8_t get_current_page();
    int8_t get_previous_page();
    Page* get_page(int8_t page_number);
    void initialize();
    ~HXPages();

protected:
    int8_t _current_page = 0;
    int8_t _previous_page = 0;
    Page *_pages[10];


};



#endif