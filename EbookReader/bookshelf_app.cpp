#include "bookshelf_app.h"
#include "app_manager.h"
#include "bottom_bar.h"
#include "user_config.h"
#include "reading_app.h"
#include <Arduino.h>

// External font
LV_FONT_DECLARE(my_font_chinese_16);

// External reading app
extern ReadingApp reading_app;

const char* BookshelfApp::BOOKS_FOLDER = "/books";

BookshelfApp::BookshelfApp() : container(nullptr), book_labels(nullptr),
                               book_paths(nullptr), book_count(0),
                               selected_index(0), last_key_time(0) {
}

BookshelfApp::~BookshelfApp() {
    cleanup();
}

void BookshelfApp::cleanup() {
    if (book_labels) {
        delete[] book_labels;
        book_labels = nullptr;
    }
    
    if (book_paths) {
        for (int i = 0; i < book_count; i++) {
            if (book_paths[i]) {
                free((void*)book_paths[i]);
            }
        }
        delete[] book_paths;
        book_paths = nullptr;
    }
    
    book_count = 0;
}

void BookshelfApp::create_books_folder_if_needed() {
    if (!SD.exists(BOOKS_FOLDER)) {
        Serial.printf("Creating books folder: %s\n", BOOKS_FOLDER);
        if (SD.mkdir(BOOKS_FOLDER)) {
            Serial.println("Books folder created successfully");
        } else {
            Serial.println("Failed to create books folder");
        }
    }
}

bool BookshelfApp::is_book_file(const char* filename) {
    if (!filename) return false;
    
    int len = strlen(filename);
    if (len < 4) return false;
    
    // Check for supported file extensions
    const char* ext = filename + len - 4;
    return (strcasecmp(ext, ".txt") == 0 || 
            strcasecmp(ext, ".TXT") == 0);
}

void BookshelfApp::scan_books_folder() {
    cleanup();
    
    File dir = SD.open(BOOKS_FOLDER);
    if (!dir) {
        Serial.println("Failed to open books folder");
        return;
    }
    
    if (!dir.isDirectory()) {
        Serial.println("Books path is not a directory");
        dir.close();
        return;
    }
    
    // First pass: count books
    book_count = 0;
    File file = dir.openNextFile();
    while (file && book_count < MAX_BOOKS) {
        if (!file.isDirectory() && is_book_file(file.name())) {
            book_count++;
        }
        file.close();
        file = dir.openNextFile();
    }
    dir.close();
    
    Serial.printf("Found %d books\n", book_count);
    
    if (book_count == 0) {
        return;
    }
    
    // Allocate arrays
    book_labels = new lv_obj_t*[book_count];
    book_paths = new const char*[book_count];
    
    // Second pass: store book information
    dir = SD.open(BOOKS_FOLDER);
    int index = 0;
    file = dir.openNextFile();
    while (file && index < book_count) {
        if (!file.isDirectory() && is_book_file(file.name())) {
            // Store full path
            char* full_path = (char*)malloc(strlen(BOOKS_FOLDER) + strlen(file.name()) + 2);
            sprintf(full_path, "%s/%s", BOOKS_FOLDER, file.name());
            book_paths[index] = full_path;
            
            Serial.printf("Book %d: %s\n", index, full_path);
            index++;
        }
        file.close();
        file = dir.openNextFile();
    }
    dir.close();
}

void BookshelfApp::init() {
    Serial.println("Bookshelf app init");
    
    selected_index = 0;
    
    // Ensure books folder exists
    create_books_folder_if_needed();
    
    // Scan for books
    scan_books_folder();
    
    // Create container
    container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(container, 196, 180);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(container, lv_color_white(), 0);
    lv_obj_set_style_border_width(container, 1, 0);
    lv_obj_set_style_pad_all(container, 5, 0);
    
    // Create title
    lv_obj_t* title = lv_label_create(container);
    lv_obj_set_style_text_font(title, &my_font_chinese_16, 0);
    lv_label_set_text(title, "书架");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    if (book_count == 0) {
        // No books found message
        lv_obj_t* msg = lv_label_create(container);
        lv_obj_set_style_text_font(msg, &my_font_chinese_16, 0);
        lv_label_set_text(msg, "未找到书籍\n请将txt文件\n放入SD卡\n/books文件夹");
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);
    } else {
        // Create book list
        for (int i = 0; i < book_count && i < 5; i++) {  // Show max 5 books on screen
            book_labels[i] = lv_label_create(container);
            lv_obj_set_style_text_font(book_labels[i], &my_font_chinese_16, 0);
            
            // Extract filename from path
            const char* filename = strrchr(book_paths[i], '/');
            if (filename) {
                filename++; // Skip the '/'
            } else {
                filename = book_paths[i];
            }
            
            lv_label_set_text(book_labels[i], filename);
            lv_obj_align(book_labels[i], LV_ALIGN_TOP_LEFT, 10, 35 + i * 25);
        }
        
        update_display();
    }
    
    bottom_bar.update_app_info("Bookshelf");
}

void BookshelfApp::deinit() {
    Serial.println("Bookshelf app deinit");
    
    if (container) {
        lv_obj_del(container);
        container = nullptr;
    }
    
    cleanup();
}

void BookshelfApp::update_display() {
    if (book_count == 0) return;
    
    // Update cursor for selected book
    for (int i = 0; i < book_count && i < 5; i++) {
        if (book_labels[i]) {
            // Extract filename from path
            const char* filename = strrchr(book_paths[i], '/');
            if (filename) {
                filename++; // Skip the '/'
            } else {
                filename = book_paths[i];
            }
            
            if (i == selected_index) {
                // Selected item: add cursor
                char text_with_cursor[128];
                snprintf(text_with_cursor, sizeof(text_with_cursor), "▶ %s", filename);
                lv_label_set_text(book_labels[i], text_with_cursor);
            } else {
                // Unselected item
                lv_label_set_text(book_labels[i], filename);
            }
        }
    }
}

void BookshelfApp::select_book() {
    if (selected_index < 0 || selected_index >= book_count) {
        Serial.println("Invalid book selection");
        return;
    }
    
    // Set the book path for reading app
    reading_app.set_book_path(book_paths[selected_index]);
    
    Serial.printf("Selected book: %s\n", book_paths[selected_index]);
    
    // Switch to reading app
    app_manager.switch_to_app(2); // Reading app is at index 2 now
}

void BookshelfApp::loop() {
    if (millis() - last_key_time < 300) return; // Debounce
    
    // Navigate down (BOOT button)
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        last_key_time = millis();
        
        if (book_count > 0) {
            selected_index++;
            if (selected_index >= book_count) {
                selected_index = 0;
            }
            
            update_display();
            Serial.printf("Selected book index: %d\n", selected_index);
        }
    }
    
    // Select book (PWR button)
    if (digitalRead(PWR_BUTTON_PIN) == LOW) {
        last_key_time = millis();
        
        if (book_count > 0) {
            Serial.printf("Selecting book: %d\n", selected_index);
            select_book();
        }
    }
}

const char* BookshelfApp::get_app_info() {
    return "Bookshelf";
}

const char* BookshelfApp::get_app_name() {
    return "书架";
}
