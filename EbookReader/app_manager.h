#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "lvgl.h"

// Base class for all apps
class BaseApp {
public:
    virtual ~BaseApp() {}
    
    // Initialize app UI
    virtual void init() = 0;
    
    // Cleanup app UI
    virtual void deinit() = 0;
    
    // Handle loop logic (button presses, etc.)
    virtual void loop() = 0;
    
    // Get app-specific info for bottom bar
    virtual const char* get_app_info() = 0;
    
    // Get app name
    virtual const char* get_app_name() = 0;
};

// App Manager to handle app switching
class AppManager {
private:
    BaseApp* current_app;
    BaseApp** apps;
    int app_count;
    int current_app_index;
    
public:
    AppManager();
    ~AppManager();
    
    // Register apps
    void register_apps(BaseApp** app_list, int count);
    
    // Switch to specific app by index
    void switch_to_app(int index);
    
    // Get current app
    BaseApp* get_current_app();
    
    // Get app count
    int get_app_count();
    
    // Get app by index
    BaseApp* get_app(int index);
    
    // Call current app's loop
    void loop();
    
    // Get current app info for bottom bar
    const char* get_current_app_info();
};

// Global app manager instance
extern AppManager app_manager;

#endif
