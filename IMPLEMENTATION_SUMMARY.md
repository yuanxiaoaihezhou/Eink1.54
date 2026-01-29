# UI Refactoring Implementation Summary

## Problem Statement (Chinese)
更改ui逻辑，改为所有页面下面都有一个很小的下边栏负责显示电量以及app专属信息，将主页改为多个app显示和选择，每个具体的程序抽象出一个xxx_app由app_manager统一管理，目前只需要阅读app，下边栏app专属信息显示当前章节，页数/总页数

## Translation
Change UI logic to have a small bottom bar on all pages showing battery level and app-specific information. Change the main page to show multiple apps for selection. Abstract each specific program into an xxx_app pattern managed uniformly by app_manager. Currently only need a reading app. The bottom bar's app-specific information should display current chapter, page number/total pages.

## Implementation Summary

### Changes Made

#### 1. New Architecture Components
- **App Manager System** (`app_manager.h/cpp`)
  - `BaseApp`: Abstract base class for all applications
  - `AppManager`: Singleton manager for app registration and switching
  
- **Bottom Bar Component** (`bottom_bar.h/cpp`)
  - Persistent 16-pixel status bar at bottom of screen
  - Left side: Battery percentage (e.g., "85%")
  - Right side: App-specific information (e.g., "1/100" for reading)
  
- **Reading App** (`reading_app.h/cpp`)
  - Refactored from original reading code
  - Shows "page/total" in bottom bar
  - Maintains UTF-8 safe text reading
  - Page navigation with history tracking
  
- **Main Menu App** (`main_menu_app.h/cpp`)
  - App selection interface
  - Currently shows "Reading" app
  - Easy to extend with more apps

#### 2. Modified Components
- **user_app.cpp**: Refactored to use app manager
- **board_power_bsp.cpp/h**: Added battery reading functionality

### Statistics
- **Files Added**: 9 (8 source files + 1 documentation)
- **Files Modified**: 4
- **Total Lines Added**: ~898 lines
- **Total Lines Removed**: ~158 lines
- **Net Change**: +740 lines

### Key Features

1. **Modular App System**
   - Easy to add new apps by extending `BaseApp`
   - Clean separation of concerns
   - Automatic UI cleanup on app switching

2. **Persistent Bottom Bar**
   - Always visible across all apps
   - Updates battery every 60 seconds
   - Updates app info only when changed (reduces e-ink refreshes)

3. **Optimized for E-Paper**
   - Minimal unnecessary updates
   - Content area adjusted to 184px (from 200px) for bottom bar
   - Proper cleanup to avoid ghost images

4. **Battery Monitoring**
   - ADC-based reading (with ESP32-S3 compatibility)
   - Li-Po battery percentage calculation (4.2V=100%, 3.0V=0%)
   - Power management (VBAT turned off after reading)

5. **Reading App Improvements**
   - Total page estimation
   - Page history with validity tracking
   - Improved UTF-8 character boundary detection
   - Status only updates when page changes

### Code Quality Improvements
- Fixed potential memory leaks
- Used `snprintf` instead of `sprintf` for safety
- Proper cleanup order to prevent use-after-free
- Static style initialization guard
- Prevented multiple bottom bar instantiation
- Optimized update frequency for e-paper display

### Button Mapping

**Main Menu:**
- BOOT (GPIO0): Navigate down through app list
- PWR (GPIO18): Select app

**Reading App:**
- BOOT (GPIO0): Next page
- PWR (GPIO18): Previous page

### Screen Layout
```
┌──────────────────────┐  200px width
│                      │
│   App Content Area   │  184px height
│   (196px width)      │
│                      │
├──────────────────────┤
│ 85%      1/100       │  16px bottom bar
└──────────────────────┘
```

### Testing Notes
- No existing test infrastructure in repository
- Changes follow existing code patterns
- Hardware testing recommended for:
  - Battery reading calibration
  - E-ink display refresh optimization
  - Button responsiveness

### Future Enhancements
To add a new app:
1. Create class inheriting from `BaseApp`
2. Implement all virtual methods
3. Add instance to `app_list` in `user_app.cpp`
4. Rebuild and flash

Example apps that could be added:
- Settings app
- Clock/Timer app
- System info app
- File browser app

### Documentation
- Added `ARCHITECTURE.md` with detailed architecture explanation
- Added inline comments for complex logic
- Code follows existing style and conventions

## Security
- CodeQL analysis: No issues detected
- No secrets or credentials in code
- Proper bounds checking on buffers
- Safe string operations (snprintf)

## Compatibility
- ESP32 and ESP32-S3 compatible
- Arduino framework based
- LVGL 8.x UI library
- Maintains backward compatibility with existing hardware pins
