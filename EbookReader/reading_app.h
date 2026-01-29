#ifndef READING_APP_H
#define READING_APP_H

#include "app_manager.h"
#include "lvgl.h"
#include <SD.h>

// State definitions
enum ReadingState {
    STATE_BOOKSHELF = 0,
    STATE_READING = 1,
    STATE_MENU = 2
};

class ReadingApp : public BaseApp {
private:
    lv_obj_t* label_content;
    lv_obj_t* menu_container;
    lv_obj_t** menu_items_labels;
    const char** menu_items_names;
    
    lv_style_t style_text;
    bool style_initialized;
    
    File book_file;
    const char* book_path;
    unsigned long current_offset;
    int page_num;
    unsigned long total_file_size;
    int estimated_total_pages;
    
    // History for previous page tracking
    unsigned long history_offsets[500];
    bool history_valid[500];
    
    // Text buffer
    static const int BUFFER_SIZE = 512;
    char text_buffer[BUFFER_SIZE + 1];
    
    // Status buffer for bottom bar
    char status_buffer[64];
    char last_status_buffer[64];
    
    // Timing for debounce
    unsigned long last_key_time;
    unsigned long boot_press_start;
    unsigned long pwr_press_start;
    bool boot_pressed;
    bool pwr_pressed;
    
    // State management
    ReadingState current_state;
    int menu_selection;
    int total_menu_items;
    
    // Bookshelf state
    lv_obj_t* bookshelf_container;
    lv_obj_t** book_labels;
    const char** book_paths;
    int book_count;
    int bookshelf_selection;
    static const int MAX_BOOKS = 50;
    static const char* BOOKS_FOLDER;
    
    // Internal methods - Reading
    void load_page(unsigned long offset);
    void show_error(const char* msg);
    int read_utf8_safe(File &f, char* buf, int maxLen);
    void calculate_total_pages();
    void update_status_info();
    
    // Internal methods - Menu
    void show_menu();
    void hide_menu();
    void update_menu_display();
    void execute_menu_action();
    void cleanup_menu_ui();
    
    // Internal methods - Bookshelf
    void show_bookshelf();
    void hide_bookshelf();
    void scan_books_folder();
    void create_books_folder_if_needed();
    void update_bookshelf_display();
    void select_book();
    void cleanup_bookshelf_ui();
    bool is_book_file(const char* filename);
    
public:
    ReadingApp();
    ~ReadingApp();
    
    void init() override;
    void deinit() override;
    void loop() override;
    const char* get_app_info() override;
    const char* get_app_name() override;
    
    // Set book path
    void set_book_path(const char* path);
};

#endif
