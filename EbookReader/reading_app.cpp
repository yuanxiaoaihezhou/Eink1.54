#include "reading_app.h"
#include "bottom_bar.h"
#include "user_config.h"
#include <Arduino.h>

// External font declaration
LV_FONT_DECLARE(my_font_chinese_16);

// Menu items
static const char* MENU_ITEMS[] = {
    "返回阅读",
    "强制刷新",
    "返回书架",
    "返回主菜单"
};
static const int MENU_ITEM_COUNT = 4;

const char* ReadingApp::BOOKS_FOLDER = "/books";

ReadingApp::ReadingApp() : label_content(nullptr), menu_container(nullptr),
                           menu_items_labels(nullptr), menu_items_names(nullptr),
                           style_initialized(false),
                           book_path("/book.txt"), 
                           current_offset(0), page_num(1), total_file_size(0),
                           estimated_total_pages(1), last_key_time(0),
                           boot_press_start(0), pwr_press_start(0),
                           boot_pressed(false), pwr_pressed(false),
                           current_state(STATE_BOOKSHELF), menu_selection(0), total_menu_items(0),
                           bookshelf_container(nullptr), book_labels(nullptr),
                           book_paths(nullptr), book_count(0), bookshelf_selection(0) {
    memset(history_offsets, 0, sizeof(history_offsets));
    memset(history_valid, 0, sizeof(history_valid));
    memset(text_buffer, 0, sizeof(text_buffer));
    memset(status_buffer, 0, sizeof(status_buffer));
    memset(last_status_buffer, 0, sizeof(last_status_buffer));
}

ReadingApp::~ReadingApp() {
    cleanup_menu_ui();
    cleanup_bookshelf_ui();
}

void ReadingApp::set_book_path(const char* path) {
    book_path = path;
}

void ReadingApp::init() {
    Serial.println("Reading app init - starting with bookshelf");
    
    // Initialize state to bookshelf
    current_state = STATE_BOOKSHELF;
    menu_selection = 0;
    total_menu_items = 0;
    bookshelf_selection = 0;
    
    // Initialize text style
    if (!style_initialized) {
        lv_style_init(&style_text);
        lv_style_set_text_font(&style_text, &my_font_chinese_16);
        lv_style_set_text_color(&style_text, lv_color_black());
        lv_style_set_text_line_space(&style_text, 4);
        style_initialized = true;
    }
    
    // Create label_content but hide it initially (will be shown when entering reading state)
    label_content = lv_label_create(lv_scr_act());
    lv_obj_add_style(label_content, &style_text, 0);
    lv_obj_set_width(label_content, 196);
    lv_obj_set_height(label_content, 180);
    lv_obj_align(label_content, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_long_mode(label_content, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(label_content, LV_OBJ_FLAG_HIDDEN);
    
    // Show bookshelf
    show_bookshelf();
}

void ReadingApp::deinit() {
    Serial.println("Reading app deinit");
    
    if (book_file) {
        book_file.close();
    }
    
    if (menu_container) {
        lv_obj_del(menu_container);
        menu_container = nullptr;
    }
    
    if (bookshelf_container) {
        lv_obj_del(bookshelf_container);
        bookshelf_container = nullptr;
    }
    
    if (label_content) {
        lv_obj_del(label_content);
        label_content = nullptr;
    }
    
    cleanup_menu_ui();
    cleanup_bookshelf_ui();
}

void ReadingApp::cleanup_menu_ui() {
    if (menu_items_labels) {
        delete[] menu_items_labels;
        menu_items_labels = nullptr;
    }
    
    if (menu_items_names) {
        delete[] menu_items_names;
        menu_items_names = nullptr;
    }
}

void ReadingApp::show_menu() {
    if (current_state == STATE_MENU) return;
    
    current_state = STATE_MENU;
    menu_selection = 0;
    total_menu_items = MENU_ITEM_COUNT;
    
    // Hide reading content
    if (label_content) {
        lv_obj_add_flag(label_content, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Create menu container
    menu_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(menu_container, 196, 180);
    lv_obj_align(menu_container, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(menu_container, lv_color_white(), 0);
    lv_obj_set_style_border_width(menu_container, 1, 0);
    lv_obj_set_style_pad_all(menu_container, 5, 0);
    
    // Create title
    lv_obj_t* title = lv_label_create(menu_container);
    lv_obj_set_style_text_font(title, &my_font_chinese_16, 0);
    lv_label_set_text(title, "系统菜单");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    // Create menu items
    menu_items_labels = new lv_obj_t*[total_menu_items];
    menu_items_names = new const char*[total_menu_items];
    
    // Check for allocation failure
    if (!menu_items_labels || !menu_items_names) {
        Serial.println("Failed to allocate menu arrays");
        cleanup_menu_ui();
        current_state = STATE_READING;
        return;
    }
    
    for (int i = 0; i < total_menu_items; i++) {
        menu_items_labels[i] = lv_label_create(menu_container);
        lv_obj_set_style_text_font(menu_items_labels[i], &my_font_chinese_16, 0);
        menu_items_names[i] = MENU_ITEMS[i];
        lv_label_set_text(menu_items_labels[i], menu_items_names[i]);
        lv_obj_align(menu_items_labels[i], LV_ALIGN_TOP_LEFT, 10, 35 + i * 25);
    }
    
    update_menu_display();
    Serial.println("Menu shown");
}

void ReadingApp::hide_menu() {
    if (current_state != STATE_MENU) return;
    
    current_state = STATE_READING;
    
    // Delete menu
    if (menu_container) {
        lv_obj_del(menu_container);
        menu_container = nullptr;
    }
    
    cleanup_menu_ui();
    
    // Show reading content
    if (label_content) {
        lv_obj_clear_flag(label_content, LV_OBJ_FLAG_HIDDEN);
    }
    
    Serial.println("Menu hidden");
}

void ReadingApp::update_menu_display() {
    // Null pointer safety check
    if (!menu_items_labels || !menu_items_names) return;
    
    // Update menu items with cursor indicator
    // No background highlighting for e-ink display
    for (int i = 0; i < total_menu_items; i++) {
        if (menu_items_labels[i] && menu_items_names[i]) {
            if (i == menu_selection) {
                // Selected item: add cursor prefix "▶ "
                char text_with_cursor[128];
                snprintf(text_with_cursor, sizeof(text_with_cursor), "▶ %s", menu_items_names[i]);
                lv_label_set_text(menu_items_labels[i], text_with_cursor);
            } else {
                // Unselected item: show name without cursor
                lv_label_set_text(menu_items_labels[i], menu_items_names[i]);
            }
        }
    }
}

void ReadingApp::execute_menu_action() {
    // Bounds check
    if (menu_selection < 0 || menu_selection >= MENU_ITEM_COUNT) {
        Serial.println("Invalid menu selection");
        return;
    }
    
    const char* selected = MENU_ITEMS[menu_selection];
    
    if (strcmp(selected, "返回阅读") == 0) {
        hide_menu();
    } else if (strcmp(selected, "强制刷新") == 0) {
        // Reload current page
        load_page(current_offset);
        hide_menu();
    } else if (strcmp(selected, "返回书架") == 0) {
        // Return to bookshelf
        hide_menu();
        // Hide reading content
        if (label_content) {
            lv_obj_add_flag(label_content, LV_OBJ_FLAG_HIDDEN);
        }
        show_bookshelf();
    } else if (strcmp(selected, "返回主菜单") == 0) {
        // Switch back to main menu (app 0)
        hide_menu();
        extern AppManager app_manager;
        app_manager.switch_to_app(0);
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
    unsigned long current_time = millis();
    
    // BOOT button handling - non-blocking
    bool boot_btn = (digitalRead(BOOT_BUTTON_PIN) == LOW);
    
    if (boot_btn && !boot_pressed) {
        // Button just pressed
        boot_pressed = true;
        boot_press_start = current_time;
    } else if (!boot_btn && boot_pressed) {
        // Button just released
        boot_pressed = false;
        unsigned long press_duration = current_time - boot_press_start;
        
        // Debounce check
        if (press_duration < 50) return; // Too short, ignore
        
        if (press_duration >= 800) {
            // Long press: toggle menu
            if (current_state == STATE_READING) {
                show_menu();
            } else if (current_state == STATE_MENU) {
                hide_menu();
            }
        } else {
            // Short press
            if (current_time - last_key_time > 300) {
                last_key_time = current_time;
                
                if (current_state == STATE_READING) {
                    // Next page in reading mode
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
                } else if (current_state == STATE_BOOKSHELF) {
                    // Navigate down in bookshelf
                    if (book_count > 0) {
                        bookshelf_selection++;
                        if (bookshelf_selection >= book_count) {
                            bookshelf_selection = 0;
                        }
                        update_bookshelf_display();
                        Serial.printf("Bookshelf selection: %d\n", bookshelf_selection);
                    }
                }
            }
        }
    }
    
    // PWR button handling - non-blocking
    bool pwr_btn = (digitalRead(PWR_BUTTON_PIN) == LOW);
    
    if (pwr_btn && !pwr_pressed) {
        // Button just pressed
        pwr_pressed = true;
        pwr_press_start = current_time;
    } else if (!pwr_btn && pwr_pressed) {
        // Button just released
        pwr_pressed = false;
        unsigned long press_duration = current_time - pwr_press_start;
        
        // Debounce check
        if (press_duration < 50) return; // Too short, ignore
        
        if (press_duration >= 800) {
            // Long press: confirm menu selection or select book
            if (current_state == STATE_MENU) {
                execute_menu_action();
            } else if (current_state == STATE_BOOKSHELF) {
                // Select book from bookshelf
                if (book_count > 0) {
                    select_book();
                }
            }
        } else {
            // Short press
            if (current_time - last_key_time > 300) {
                last_key_time = current_time;
                
                if (current_state == STATE_READING) {
                    // Previous page
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
                } else if (current_state == STATE_MENU) {
                    // Move selection down
                    menu_selection = (menu_selection + 1) % total_menu_items;
                    update_menu_display();
                }
            }
        }
    }
}

// ========== Bookshelf Methods ==========

void ReadingApp::cleanup_bookshelf_ui() {
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

void ReadingApp::create_books_folder_if_needed() {
    if (!SD.exists(BOOKS_FOLDER)) {
        Serial.printf("Creating books folder: %s\n", BOOKS_FOLDER);
        if (SD.mkdir(BOOKS_FOLDER)) {
            Serial.println("Books folder created successfully");
        } else {
            Serial.println("Failed to create books folder");
        }
    }
}

bool ReadingApp::is_book_file(const char* filename) {
    if (!filename) return false;
    
    int len = strlen(filename);
    if (len < 4) return false;
    
    // Check for supported file extensions
    const char* ext = filename + len - 4;
    return (strcasecmp(ext, ".txt") == 0 || 
            strcasecmp(ext, ".TXT") == 0);
}

void ReadingApp::scan_books_folder() {
    cleanup_bookshelf_ui();
    
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

void ReadingApp::show_bookshelf() {
    if (current_state == STATE_BOOKSHELF && bookshelf_container) return;
    
    current_state = STATE_BOOKSHELF;
    bookshelf_selection = 0;
    
    // Ensure books folder exists
    create_books_folder_if_needed();
    
    // Scan for books
    scan_books_folder();
    
    // Create container
    bookshelf_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bookshelf_container, 196, 180);
    lv_obj_align(bookshelf_container, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(bookshelf_container, lv_color_white(), 0);
    lv_obj_set_style_border_width(bookshelf_container, 1, 0);
    lv_obj_set_style_pad_all(bookshelf_container, 5, 0);
    
    // Create title
    lv_obj_t* title = lv_label_create(bookshelf_container);
    lv_obj_set_style_text_font(title, &my_font_chinese_16, 0);
    lv_label_set_text(title, "书架");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    if (book_count == 0) {
        // No books found message
        lv_obj_t* msg = lv_label_create(bookshelf_container);
        lv_obj_set_style_text_font(msg, &my_font_chinese_16, 0);
        lv_label_set_text(msg, "未找到书籍\n请将txt文件\n放入SD卡\n/books文件夹");
        lv_obj_align(msg, LV_ALIGN_CENTER, 0, 0);
    } else {
        // Create book list (show max 5 books on screen)
        for (int i = 0; i < book_count && i < 5; i++) {
            book_labels[i] = lv_label_create(bookshelf_container);
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
        
        update_bookshelf_display();
    }
    
    bottom_bar.update_app_info("Bookshelf");
}

void ReadingApp::hide_bookshelf() {
    if (bookshelf_container) {
        lv_obj_del(bookshelf_container);
        bookshelf_container = nullptr;
    }
}

void ReadingApp::update_bookshelf_display() {
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
            
            if (i == bookshelf_selection) {
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

void ReadingApp::select_book() {
    if (bookshelf_selection < 0 || bookshelf_selection >= book_count) {
        Serial.println("Invalid book selection");
        return;
    }
    
    // Set the book path
    book_path = book_paths[bookshelf_selection];
    
    Serial.printf("Selected book: %s\n", book_path);
    
    // Hide bookshelf
    hide_bookshelf();
    
    // Show reading content
    if (label_content) {
        lv_obj_clear_flag(label_content, LV_OBJ_FLAG_HIDDEN);
    }
    
    // Switch to reading state
    current_state = STATE_READING;
    
    // Calculate total pages
    calculate_total_pages();
    
    // Load first page
    current_offset = 0;
    page_num = 1;
    load_page(current_offset);
}

const char* ReadingApp::get_app_info() {
    return status_buffer;
}

const char* ReadingApp::get_app_name() {
    return "Reading";
}
