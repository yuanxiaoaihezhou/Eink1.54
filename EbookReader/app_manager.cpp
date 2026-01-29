#include "app_manager.h"
#include <Arduino.h>

AppManager app_manager;

AppManager::AppManager() : current_app(nullptr), apps(nullptr), app_count(0), current_app_index(-1) {
}

AppManager::~AppManager() {
    if (current_app) {
        current_app->deinit();
    }
}

void AppManager::register_apps(BaseApp** app_list, int count) {
    apps = app_list;
    app_count = count;
}

void AppManager::switch_to_app(int index) {
    if (index < 0 || index >= app_count) {
        Serial.printf("Invalid app index: %d\n", index);
        return;
    }
    
    // Deinit current app if exists
    if (current_app) {
        Serial.printf("Deinit app: %s\n", current_app->get_app_name());
        current_app->deinit();
    }
    
    // Switch to new app
    current_app_index = index;
    current_app = apps[index];
    
    Serial.printf("Init app: %s\n", current_app->get_app_name());
    current_app->init();
}

BaseApp* AppManager::get_current_app() {
    return current_app;
}

int AppManager::get_app_count() {
    return app_count;
}

BaseApp* AppManager::get_app(int index) {
    if (index < 0 || index >= app_count) {
        return nullptr;
    }
    return apps[index];
}

void AppManager::loop() {
    if (current_app) {
        current_app->loop();
    }
}

const char* AppManager::get_current_app_info() {
    if (current_app) {
        return current_app->get_app_info();
    }
    return "";
}
