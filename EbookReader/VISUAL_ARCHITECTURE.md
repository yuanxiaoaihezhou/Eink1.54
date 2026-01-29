# Visual Architecture Diagram

## Component Hierarchy

```
┌─────────────────────────────────────────────────────────────────┐
│                         EbookReader.ino                         │
│  (Main Arduino sketch - initializes hardware and LVGL)         │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                         user_app.cpp                            │
│  • Initializes hardware (SD, E-ink, Power, Buttons)            │
│  • Creates app instances                                        │
│  • Registers apps with AppManager                              │
│  • Creates bottom bar                                           │
│  • Main loop handling                                           │
└───────────┬─────────────────────────────────┬───────────────────┘
            │                                 │
            ▼                                 ▼
┌───────────────────────┐         ┌─────────────────────────┐
│    AppManager         │         │     BottomBar           │
│  (app_manager.cpp)    │         │  (bottom_bar.cpp)       │
│                       │         │                         │
│  • Manages app list   │         │  • Battery: "85%"       │
│  • Switches apps      │         │  • App info: "1/100"    │
│  • Delegates to apps  │         │  • Always visible       │
└───────┬───────────────┘         └─────────────────────────┘
        │
        │  Manages
        ▼
┌─────────────────────────────────────────────────────────────────┐
│                        BaseApp (Abstract)                        │
│                      (app_manager.h)                            │
│                                                                  │
│  Interface:                                                      │
│  • init()        - Setup UI                                     │
│  • deinit()      - Cleanup UI                                   │
│  • loop()        - Handle input                                 │
│  • get_app_info()- Return status text                          │
│  • get_app_name()- Return app name                             │
└──────────────────────┬──────────────────────────────────────────┘
                       │
           ┌───────────┴───────────┐
           │                       │
           ▼                       ▼
┌──────────────────────┐  ┌──────────────────────┐
│   MainMenuApp        │  │    ReadingApp        │
│ (main_menu_app.cpp)  │  │  (reading_app.cpp)   │
│                      │  │                      │
│  • Shows app list    │  │  • Displays book     │
│  • Navigate: BOOT    │  │  • UTF-8 safe read   │
│  • Select: PWR       │  │  • Page navigation   │
│  • Info: "Menu"      │  │  • Info: "page/tot"  │
└──────────────────────┘  └──────────────────────┘
```

## Data Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                         User Input                               │
│                    (BOOT / PWR buttons)                          │
└────────────────────────────┬─────────────────────────────────────┘
                             ▼
                   ┌─────────────────────┐
                   │  reader_loop_handle │
                   │   (user_app.cpp)    │
                   └──────────┬──────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌──────────────┐    ┌──────────────────┐    ┌──────────────┐
│  Battery     │    │   app_manager    │    │  Bottom Bar  │
│  Update      │    │     .loop()      │    │   Update     │
│  (60s)       │    │                  │    │  (on change) │
└──────┬───────┘    └────────┬─────────┘    └──────┬───────┘
       │                     │                      │
       │                     ▼                      │
       │            ┌─────────────────┐             │
       │            │  Current App    │             │
       │            │    .loop()      │             │
       │            └────────┬────────┘             │
       │                     │                      │
       │                     ▼                      │
       │            ┌─────────────────┐             │
       │            │ Process Input   │             │
       │            │ Update State    │             │
       │            └────────┬────────┘             │
       │                     │                      │
       └─────────────────────┴──────────────────────┘
                             │
                             ▼
                    ┌─────────────────┐
                    │  Update UI      │
                    │  (LVGL)         │
                    └────────┬────────┘
                             ▼
                    ┌─────────────────┐
                    │  E-ink Display  │
                    │  Refresh        │
                    └─────────────────┘
```

## Screen State Machine

```
                    ┌──────────────┐
                    │   System     │
                    │   Boot       │
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
                    │  Main Menu   │◄──────────┐
                    │              │           │
                    │ Apps:        │           │
                    │ > Reading    │           │
                    │              │           │
                    └──────┬───────┘           │
                           │                   │
                      [PWR Button]             │
                           │                   │
                           ▼                   │
                    ┌──────────────┐           │
                    │ Reading App  │           │
                    │              │     [Back to menu]
                    │ Page: 1/100  │    (future feature)
                    │              │           │
                    │ [BOOT]=Next  │           │
                    │ [PWR]=Prev   │           │
                    └──────────────┘───────────┘
```

## File Organization

```
EbookReader/
├── EbookReader.ino          # Main sketch
├── user_app.cpp             # Hardware init & main loop
├── user_app.h               # Function declarations
├── user_config.h            # Pin definitions
│
├── app_manager.h            # App system interface
├── app_manager.cpp          # App system implementation
│
├── bottom_bar.h             # Status bar interface
├── bottom_bar.cpp           # Status bar implementation
│
├── main_menu_app.h          # Menu app interface
├── main_menu_app.cpp        # Menu app implementation
│
├── reading_app.h            # Reading app interface
├── reading_app.cpp          # Reading app implementation
│
├── ARCHITECTURE.md          # Architecture docs
├── IMPLEMENTATION_SUMMARY.md # Change summary
│
└── src/
    ├── power/
    │   ├── board_power_bsp.h
    │   └── board_power_bsp.cpp  # Battery reading
    └── display/
        └── epaper_driver_bsp.*   # E-ink driver
```

## Memory Layout (Approximate)

```
┌─────────────────────────────────────────┐
│           ESP32-S3 Memory                │
├─────────────────────────────────────────┤
│  PSRAM (8MB)                             │
│  ├─ LVGL Display Buffer (~78KB)         │
│  └─ Available for apps                  │
├─────────────────────────────────────────┤
│  SRAM                                    │
│  ├─ App Manager (~100 bytes)            │
│  ├─ Bottom Bar (~200 bytes)             │
│  ├─ MainMenuApp (~300 bytes)            │
│  ├─ ReadingApp (~2KB)                   │
│  │   └─ Text buffer: 512 bytes          │
│  │   └─ History: 500*4 = 2KB            │
│  └─ Other system memory                 │
└─────────────────────────────────────────┘
```

## Bottom Bar Layout Detail

```
┌─────────────────────────────────────────┐
│ 85%                          1/100      │  16px height
└─────────────────────────────────────────┘
  ↑                                   ↑
  Battery                      App-specific
  (10px font)                  (10px font)
  Left aligned                 Right aligned
  Updates: 60s                 Updates: on change
```
