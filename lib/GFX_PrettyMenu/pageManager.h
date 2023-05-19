#ifndef _PAGE_MANAGER_H_
#define _PAGE_MANAGER_H_

#pragma once
#include <Arduino.h>
#include <Simple_GFX.h>

/*
Manages the pages for the project. Initiate a derived Page class from the main.cpp file and add the Page object to the Manager via add_page()
*/

#define MAX_PAGES 10        // Can change this value depending on how many pages you need, more pages = more memory usage

enum PageNumber {
    MAIN_PAGE,
    INLET_TEMP_PAGE,
    PIEZO_CONFIG_PAGE,
    HEATFLUX_PAGE
};

template <typename TFTClass>
class PageManager {
public:
    PageManager(){};

    // Add a page to the page manager and assigne it an index
    void add_page(PageNumber index, Page<TFTClass>* page){
        pages[index] = page;
    };

    void set_page(PageNumber index){
        // If there's a current page, update it before switching
        if (current_page != nullptr){
            current_page->update();  
            // Save the index of the current page before switching, so it can be called if there's a "BACK" button
            for (int i = 0; i < MAX_PAGES; ++i) {
                if (current_page == pages[i]) {
                    previous_page_number = static_cast<PageNumber>(i);  // Save the index of the current page before switching (as a PageNumber type)
                    break;
                }
            }  
        }
        // Set the pointer to current page as the page at the index specified
        current_page = pages[index];
        // Initialize the page via the page's custom initialize function (if it exists)
        if (current_page != nullptr) {
            current_page->initialize();
        }
    };

    void update_current_page(){
        if (current_page != nullptr)
            current_page->update();
    };

    PageNumber get_previous_page(){
        return previous_page_number;
    };

    ~PageManager(){};

private:
    Page<TFTClass>* pages[MAX_PAGES];
    Page<TFTClass>* current_page;
    PageNumber previous_page_number;
};

#endif