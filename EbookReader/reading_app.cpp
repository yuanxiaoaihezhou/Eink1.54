#include "reading_app.h"
#include "bottom_bar.h"
#include "user_config.h"
#include <Arduino.h>

// External font declaration
LV_FONT_DECLARE(my_font_chinese_16);

ReadingApp::ReadingApp() : label_content(nullptr), style_initialized(false),
                           book_path("/book.txt"), 
                           current_offset(0), page_num(1), total_file_size(0),
                           estimated_total_pages(1), last_key_time(0) {
    memset(history_offsets, 0, sizeof(history_offsets));
    memset(history_valid, 0, sizeof(history_valid));
    memset(text_buffer, 0, sizeof(text_buffer));
    memset(status_buffer, 0, sizeof(status_buffer));
    memset(last_status_buffer, 0, sizeof(last_status_buffer));
}

ReadingApp::~ReadingApp() {
}

void ReadingApp::set_book_path(const char* path) {
    book_path = path;
}

void ReadingApp::init() {
    Serial.println("Reading app init");
    
    // Create text content area (leave space for bottom bar - 16 pixels)
    if (!style_initialized) {
        lv_style_init(&style_text);
        lv_style_set_text_font(&style_text, &my_font_chinese_16);
        lv_style_set_text_color(&style_text, lv_color_black());
        lv_style_set_text_line_space(&style_text, 4);
        style_initialized = true;
    }
    
    label_content = lv_label_create(lv_scr_act());
    lv_obj_add_style(label_content, &style_text, 0);
    lv_obj_set_width(label_content, 196);
    lv_obj_set_height(label_content, 180); // 200 - 16 (bottom bar) - 4 (padding)
    lv_obj_align(label_content, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_long_mode(label_content, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label_content, "正在加载...");
    
    // Calculate total pages
    calculate_total_pages();
    
    // Load first page
    current_offset = 0;
    page_num = 1;
    load_page(current_offset);
}

void ReadingApp::deinit() {
    Serial.println("Reading app deinit");
    
    if (book_file) {
        book_file.close();
    }
    
    if (label_content) {
        lv_obj_del(label_content);
        label_content = nullptr;
    }
}

void ReadingApp::calculate_total_pages() {
    if (!SD.exists(book_path)) {
        estimated_total_pages = 1;
        return;
    }
    
    File f = SD.open(book_path, FILE_READ);
    if (f) {
        total_file_size = f.size();
        f.close();
        
        // Estimate: each page shows approximately 350 bytes
        estimated_total_pages = (total_file_size / 350) + 1;
        if (estimated_total_pages < 1) estimated_total_pages = 1;
        
        Serial.printf("Total file size: %lu, Estimated pages: %d\n", 
                      total_file_size, estimated_total_pages);
    } else {
        estimated_total_pages = 1;
    }
}

int ReadingApp::read_utf8_safe(File &f, char* buf, int maxLen) {
    int readLen = f.readBytes(buf, maxLen);
    if (readLen <= 0) return 0;
    
    int safeLen = readLen;
    
    // Check if we cut off a multi-byte UTF-8 character
    // Continuation bytes have pattern 10xxxxxx (0x80 to 0xBF)
    while (safeLen > 0 && (buf[safeLen-1] & 0xC0) == 0x80) {
        safeLen--;
    }
    
    // Check if the last byte is a multi-byte character start that may be incomplete
    if (safeLen > 0) {
        unsigned char lastChar = (unsigned char)buf[safeLen-1];
        
        // Check if it's a multi-byte character start (bit pattern 11xxxxxx)
        // and we don't have the full character
        if ((lastChar & 0xE0) == 0xC0) {
            // 2-byte character start (110xxxxx), need 1 more byte
            if (safeLen >= readLen) safeLen--; // Incomplete, remove it
        } else if ((lastChar & 0xF0) == 0xE0) {
            // 3-byte character start (1110xxxx), need 2 more bytes
            if (safeLen >= readLen - 1) safeLen--; // Incomplete, remove it
        } else if ((lastChar & 0xF8) == 0xF0) {
            // 4-byte character start (11110xxx), need 3 more bytes
            if (safeLen >= readLen - 2) safeLen--; // Incomplete, remove it
        }
    }
    
    int backtrack = readLen - safeLen;
    if (backtrack > 0) {
        f.seek(f.position() - backtrack);
    }
    
    buf[safeLen] = '\0';
    return safeLen;
}

void ReadingApp::load_page(unsigned long offset) {
    if (!SD.exists(book_path)) {
        show_error("No book file");
        return;
    }
    
    book_file = SD.open(book_path, FILE_READ);
    if (book_file) {
        book_file.seek(offset);
        read_utf8_safe(book_file, text_buffer, BUFFER_SIZE - 1);
        
        lv_label_set_text(label_content, text_buffer);
        
        update_status_info();
        
        Serial.printf("Page %d/%d loaded. Offset: %lu\n", 
                      page_num, estimated_total_pages, offset);
        
        book_file.close();
    }
}

void ReadingApp::update_status_info() {
    // Format: "page/total"
    snprintf(status_buffer, sizeof(status_buffer), "%d/%d", page_num, estimated_total_pages);
    
    // Only update bottom bar if status changed
    if (strcmp(status_buffer, last_status_buffer) != 0) {
        bottom_bar.update_app_info(status_buffer);
        strncpy(last_status_buffer, status_buffer, sizeof(last_status_buffer) - 1);
        last_status_buffer[sizeof(last_status_buffer) - 1] = '\0';
    }
}

void ReadingApp::show_error(const char* msg) {
    if (label_content) {
        lv_label_set_text(label_content, msg);
    }
}

void ReadingApp::loop() {
    if (millis() - last_key_time < 300) return; // Debounce
    
    // Next page (BOOT button)
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        last_key_time = millis();
        
        // Save history with validity flag
        if (page_num < 500) {
            history_offsets[page_num] = current_offset;
            history_valid[page_num] = true;
        }
        
        // Move forward ~350 bytes
        current_offset += 350;
        page_num++;
        
        // Clamp to max pages
        if (page_num > estimated_total_pages) {
            page_num = estimated_total_pages;
            current_offset = total_file_size > 350 ? total_file_size - 350 : 0;
        }
        
        load_page(current_offset);
    }
    
    // Previous page (PWR button)
    if (digitalRead(PWR_BUTTON_PIN) == LOW) {
        last_key_time = millis();
        
        if (page_num > 1) {
            page_num--;
            
            // Use history if available and valid
            if (page_num < 500 && history_valid[page_num]) {
                current_offset = history_offsets[page_num];
            } else {
                if (current_offset > 350) {
                    current_offset -= 350;
                } else {
                    current_offset = 0;
                }
            }
            
            load_page(current_offset);
        }
    }
}

const char* ReadingApp::get_app_info() {
    return status_buffer;
}

const char* ReadingApp::get_app_name() {
    return "Reading";
}
