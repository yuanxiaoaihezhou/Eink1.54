#ifndef BOOKSHELF_APP_H
#define BOOKSHELF_APP_H

#include "app_manager.h"
#include "lvgl.h"
#include <SD.h>

class BookshelfApp : public BaseApp {
private:
    lv_obj_t* container;
    lv_obj_t** book_labels;
    const char** book_paths;
    int book_count;
    int selected_index;
    unsigned long last_key_time;
    
    static const int MAX_BOOKS = 50;
    static const char* BOOKS_FOLDER;
    
    // Helper methods
    void scan_books_folder();
    void create_books_folder_if_needed();
    void update_display();
    void select_book();
    void cleanup();
    bool is_book_file(const char* filename);
    
public:
    BookshelfApp();
    ~BookshelfApp();
    
    void init() override;
    void deinit() override;
    void loop() override;
    const char* get_app_info() override;
    const char* get_app_name() override;
};

#endif
