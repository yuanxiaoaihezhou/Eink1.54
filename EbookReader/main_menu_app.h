#ifndef MAIN_MENU_APP_H
#define MAIN_MENU_APP_H

#include "app_manager.h"
#include "lvgl.h"

class MainMenuApp : public BaseApp {
private:
    lv_obj_t* menu_container;
    lv_obj_t** app_labels;
    int app_count;
    int selected_index;
    
    unsigned long last_key_time;
    
    void update_menu_display();
    void select_app();
    
public:
    MainMenuApp();
    ~MainMenuApp();
    
    void init() override;
    void deinit() override;
    void loop() override;
    const char* get_app_info() override;
    const char* get_app_name() override;
};

#endif
