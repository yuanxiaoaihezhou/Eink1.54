#include "main_menu_app.h"
#include "app_manager.h"
#include "bottom_bar.h"
#include "user_config.h"
#include <Arduino.h>

// External font
LV_FONT_DECLARE(my_font_chinese_16);

MainMenuApp::MainMenuApp() : menu_container(nullptr), app_labels(nullptr),
                             app_count(0), selected_index(0), last_key_time(0) {
}

MainMenuApp::~MainMenuApp() {
    if (app_labels) {
        delete[] app_labels;
        app_labels = nullptr;
    }
}

void MainMenuApp::init() {
    Serial.println("Main menu app init");
    
    // Get app count (excluding main menu itself which is at index 0)
    app_count = app_manager.get_app_count() - 1;
    selected_index = 0;
    
    // Create container for menu (leave space for bottom bar)
    menu_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(menu_container, 196, 180);
    lv_obj_align(menu_container, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_bg_color(menu_container, lv_color_white(), 0);
    lv_obj_set_style_border_width(menu_container, 1, 0);
    lv_obj_set_style_pad_all(menu_container, 5, 0);
    
    // Create title
    lv_obj_t* title = lv_label_create(menu_container);
    lv_obj_set_style_text_font(title, &my_font_chinese_16, 0);
    lv_label_set_text(title, "选择应用");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);
    
    // Create app list labels
    if (app_count > 0) {
        app_labels = new lv_obj_t*[app_count];
        
        for (int i = 0; i < app_count; i++) {
            app_labels[i] = lv_label_create(menu_container);
            lv_obj_set_style_text_font(app_labels[i], &my_font_chinese_16, 0);
            
            // Get app name (index + 1 because main menu is at 0)
            BaseApp* app = app_manager.get_app(i + 1);
            if (app) {
                lv_label_set_text(app_labels[i], app->get_app_name());
            }
            
            lv_obj_align(app_labels[i], LV_ALIGN_TOP_LEFT, 10, 35 + i * 25);
        }
    }
    
    update_menu_display();
    bottom_bar.update_app_info("Menu");
}

void MainMenuApp::deinit() {
    Serial.println("Main menu app deinit");
    
    if (menu_container) {
        lv_obj_del(menu_container);
        menu_container = nullptr;
    }
    
    if (app_labels) {
        delete[] app_labels;
        app_labels = nullptr;
    }
}

void MainMenuApp::update_menu_display() {
    // Highlight selected app
    for (int i = 0; i < app_count; i++) {
        if (app_labels[i]) {
            if (i == selected_index) {
                lv_obj_set_style_text_color(app_labels[i], lv_color_black(), 0);
                lv_obj_set_style_bg_color(app_labels[i], lv_color_hex(0xCCCCCC), 0);
                lv_obj_set_style_bg_opa(app_labels[i], LV_OPA_COVER, 0);
            } else {
                lv_obj_set_style_text_color(app_labels[i], lv_color_black(), 0);
                lv_obj_set_style_bg_opa(app_labels[i], LV_OPA_TRANSP, 0);
            }
        }
    }
}

void MainMenuApp::select_app() {
    // Switch to selected app (offset by 1 because main menu is at index 0)
    int app_index = selected_index + 1;
    app_manager.switch_to_app(app_index);
}

void MainMenuApp::loop() {
    if (millis() - last_key_time < 300) return; // Debounce
    
    // Navigate down (BOOT button)
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        last_key_time = millis();
        
        selected_index++;
        if (selected_index >= app_count) {
            selected_index = 0;
        }
        
        update_menu_display();
        Serial.printf("Selected: %d\n", selected_index);
    }
    
    // Select app (PWR button)
    if (digitalRead(PWR_BUTTON_PIN) == LOW) {
        last_key_time = millis();
        
        if (app_count > 0) {
            Serial.printf("Selecting app: %d\n", selected_index);
            select_app();
        }
    }
}

const char* MainMenuApp::get_app_info() {
    return "Menu";
}

const char* MainMenuApp::get_app_name() {
    return "Main Menu";
}
