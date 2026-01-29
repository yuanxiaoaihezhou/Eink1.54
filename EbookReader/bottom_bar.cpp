#include "bottom_bar.h"
#include <Arduino.h>
#include <stdio.h>

BottomBar bottom_bar;

BottomBar::BottomBar() : bar_container(nullptr), battery_label(nullptr), 
                         app_info_label(nullptr), battery_percentage(100) {
    strcpy(battery_text, "100%");
    strcpy(app_info_text, "");
}

BottomBar::~BottomBar() {
    // LVGL objects are cleaned up automatically
}

void BottomBar::create() {
    // Check if already created to prevent multiple instantiation
    if (bar_container != nullptr) {
        Serial.println("Bottom bar already created");
        return;
    }
    
    // Create a container for the bottom bar (height: 16 pixels)
    bar_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bar_container, 200, 16);
    lv_obj_align(bar_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bar_container, lv_color_white(), 0);
    lv_obj_set_style_border_width(bar_container, 0, 0);
    lv_obj_set_style_pad_all(bar_container, 0, 0);
    
    // Battery label on the left
    battery_label = lv_label_create(bar_container);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_16, 0);
    lv_obj_align(battery_label, LV_ALIGN_LEFT_MID, 2, 0);
    lv_label_set_text(battery_label, battery_text);
    
    // App info label on the right
    app_info_label = lv_label_create(bar_container);
    lv_obj_set_style_text_font(app_info_label, &lv_font_montserrat_16, 0);
    lv_obj_align(app_info_label, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_label_set_text(app_info_label, app_info_text);
    
    Serial.println("Bottom bar created");
}

void BottomBar::update_battery(int percentage) {
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    
    battery_percentage = percentage;
    snprintf(battery_text, sizeof(battery_text), "%d%%", percentage);
    
    if (battery_label) {
        lv_label_set_text(battery_label, battery_text);
    }
}

void BottomBar::update_app_info(const char* info) {
    if (info) {
        strncpy(app_info_text, info, sizeof(app_info_text) - 1);
        app_info_text[sizeof(app_info_text) - 1] = '\0';
    } else {
        app_info_text[0] = '\0';
    }
    
    if (app_info_label) {
        lv_label_set_text(app_info_label, app_info_text);
    }
}

void BottomBar::show() {
    if (bar_container) {
        lv_obj_clear_flag(bar_container, LV_OBJ_FLAG_HIDDEN);
    }
}

void BottomBar::hide() {
    if (bar_container) {
        lv_obj_add_flag(bar_container, LV_OBJ_FLAG_HIDDEN);
    }
}
