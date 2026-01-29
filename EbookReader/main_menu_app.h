#ifndef MAIN_MENU_APP_H
#define MAIN_MENU_APP_H

#include "app_manager.h"
#include "lvgl.h"

class MainMenuApp : public BaseApp {
private:
    lv_obj_t* menu_container;
    lv_obj_t** menu_items_labels;
    int total_menu_items;
    int selected_index;
    
    unsigned long last_key_time;
    
    void update_menu_display();
    void select_menu_item();
    
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
