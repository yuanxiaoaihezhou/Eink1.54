#ifndef BOTTOM_BAR_H
#define BOTTOM_BAR_H

#include "lvgl.h"

class BottomBar {
private:
    lv_obj_t* bar_container;
    lv_obj_t* battery_label;
    lv_obj_t* app_info_label;
    
    int battery_percentage;
    char battery_text[16];
    char app_info_text[64];
    
public:
    BottomBar();
    ~BottomBar();
    
    // Create the bottom bar UI
    void create();
    
    // Update battery level (0-100)
    void update_battery(int percentage);
    
    // Update app-specific info
    void update_app_info(const char* info);
    
    // Show/hide bottom bar
    void show();
    void hide();
};

// Global bottom bar instance
extern BottomBar bottom_bar;

#endif
