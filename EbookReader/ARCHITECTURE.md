# UI Architecture Refactoring

## Overview
This refactoring changes the UI architecture to support multiple applications with a unified bottom status bar.

## New Components

### 1. App Manager (`app_manager.h/cpp`)
- **BaseApp**: Abstract base class for all applications
  - `init()`: Initialize app UI
  - `deinit()`: Cleanup app UI
  - `loop()`: Handle app logic (button presses, updates)
  - `get_app_info()`: Get app-specific info for bottom bar
  - `get_app_name()`: Get app name for menu

- **AppManager**: Manages all registered apps
  - Registers apps
  - Switches between apps
  - Delegates loop calls to current app

### 2. Bottom Bar (`bottom_bar.h/cpp`)
- Displays on all screens (16 pixels height)
- Shows battery percentage on left
- Shows app-specific info on right
- Updates periodically

### 3. Reading App (`reading_app.h/cpp`)
- Refactored from original reading logic
- Shows current page and total pages in bottom bar
- Handles page navigation with BOOT (next) and PWR (previous) buttons
- UTF-8 safe text reading

### 4. Main Menu App (`main_menu_app.h/cpp`)
- Displays list of available apps
- Navigate with BOOT button (down)
- Select with PWR button
- Shows "Menu" in bottom bar

## Screen Layout

```
┌──────────────────────┐
│                      │
│   App Content Area   │
│   (184 pixels high)  │
│                      │
├──────────────────────┤
│ 85%      Menu/1/100  │  <- Bottom Bar (16px)
└──────────────────────┘
```

## Button Mapping

### Main Menu:
- **BOOT (GPIO0)**: Navigate down through app list
- **PWR (GPIO18)**: Select current app

### Reading App:
- **BOOT (GPIO0)**: Next page
- **PWR (GPIO18)**: Previous page

## Battery Reading
- Added `read_battery_percentage()` to `board_power_bsp`
- Returns 0-100% based on ADC reading
- Updates every 60 seconds
- Currently returns default value (85%) - needs hardware-specific calibration

## How to Add New Apps

1. Create new app class inheriting from `BaseApp`
2. Implement all virtual methods
3. Create app instance in `user_app.cpp`
4. Add to `app_list` array
5. Rebuild and flash

Example:
```cpp
class MyApp : public BaseApp {
public:
    void init() override { /* setup UI */ }
    void deinit() override { /* cleanup */ }
    void loop() override { /* handle input */ }
    const char* get_app_info() override { return "MyInfo"; }
    const char* get_app_name() override { return "MyApp"; }
};
```

## Dependencies
- LVGL for UI
- Arduino framework
- ESP32 FreeRTOS
- SD card library

## Notes
- Content area height reduced from 200px to 184px to accommodate bottom bar
- All apps must clean up their UI in `deinit()`
- App switching automatically handles init/deinit
- Bottom bar is persistent across all apps
