#ifndef READING_APP_H
#define READING_APP_H

#include "app_manager.h"
#include "lvgl.h"
#include <SD.h>

class ReadingApp : public BaseApp {
private:
    lv_obj_t* label_content;
    
    File book_file;
    const char* book_path;
    unsigned long current_offset;
    int page_num;
    unsigned long total_file_size;
    int estimated_total_pages;
    
    // History for previous page tracking
    unsigned long history_offsets[500];
    
    // Text buffer
    static const int BUFFER_SIZE = 512;
    char text_buffer[BUFFER_SIZE + 1];
    
    // Status buffer for bottom bar
    char status_buffer[64];
    
    // Timing for debounce
    unsigned long last_key_time;
    
    // Internal methods
    void load_page(unsigned long offset);
    void show_error(const char* msg);
    int read_utf8_safe(File &f, char* buf, int maxLen);
    void calculate_total_pages();
    void update_status_info();
    
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
