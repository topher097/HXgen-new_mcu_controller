// #include <pageManager.h>


// // Add a page to the page manager and assigne it an index
// void PageManager::add_page(PageNumber index, Page* page) {
//     pages[index] = page;
// }

// // Set the current page to the page at the index specified
// void PageManager::set_page(PageNumber index) {
//     // If there's a current page, update it before switching
//     if (current_page != nullptr){
//         current_page->update();  
//         // Save the index of the current page before switching, so it can be called if there's a "BACK" button
//         for (int i = 0; i < MAX_PAGES; ++i) {
//             if (current_page == pages[i]) {
//                 previous_page_number = static_cast<PageNumber>(i);  // Save the index of the current page before switching (as a PageNumber type)
//                 break;
//             }
//         }  
//     }

//     // Set the pointer to current page as the page at the index specified
//     current_page = pages[index];

//     // Initialize the page via the page's custom initialize function (if it exists)
//     if (current_page != nullptr) {
//         current_page->initialize();
//     }
// }

// // Run the update function for the current page
// void PageManager::update_current_page() {
//     if (current_page != nullptr)
//         current_page->update();
// }

// // Get the previous PageNumber
// /* Can use this in a callback function, ex:
// void goto_previous_age(ButtonData& data) {
//     _manager.set_page(_manager.get_previous_page());
// */
// PageNumber PageManager::get_previous_page() {
//     return previous_page_number;
// }
