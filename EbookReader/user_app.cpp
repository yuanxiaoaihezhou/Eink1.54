#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "user_app.h"
#include "user_config.h"
#include "src/power/board_power_bsp.h"
#include "app_manager.h"
#include "bottom_bar.h"
#include "main_menu_app.h"
#include "reading_app.h"

// --- 全局对象 ---
epaper_driver_display *driver = NULL;
board_power_bsp_t board_div(EPD_PWR_PIN, Audio_PWR_PIN, VBAT_PWR_PIN);

// App instances
MainMenuApp main_menu;
ReadingApp reading_app;

// App list
BaseApp* app_list[] = {
    &main_menu,
    &reading_app
};

const int APP_COUNT = sizeof(app_list) / sizeof(app_list[0]);

// Battery update timer
unsigned long last_battery_update = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 60000; // Update every 60 seconds

// --- 初始化硬件 ---
void user_app_init(void)
{
  Serial.begin(115200);
  Serial.println(">>> System Booting...");

  // 1. 电源开启
  board_div.POWEER_EPD_ON();
  board_div.POWEER_Audio_ON();
  delay(100);

  // 2. 按键初始化
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PWR_BUTTON_PIN, INPUT_PULLUP); // 假设 GPIO18 需要上拉

  // 3. SD 卡初始化
  // 必须指定引脚，防止 S3 使用默认的其他引脚
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, -1);
  if(!SD.begin(-1)){ // 传入 -1 或具体 CS 引脚
      Serial.println(">>> SD Mount FAILED");
  } else {
      Serial.println(">>> SD Mount SUCCESS");
  }

  // 4. 墨水屏底层驱动初始化
  custom_lcd_spi_t driver_config = {};
    driver_config.cs = EPD_CS_PIN;
    driver_config.dc = EPD_DC_PIN;
    driver_config.rst = EPD_RST_PIN;
    driver_config.busy = EPD_BUSY_PIN;
    driver_config.mosi = EPD_MOSI_PIN;
    driver_config.scl = EPD_SCK_PIN;
    driver_config.spi_host = EPD_SPI_NUM;
    driver_config.buffer_len = 5000;
  
  driver = new epaper_driver_display(EPD_WIDTH, EPD_HEIGHT, driver_config);
  driver->EPD_Init();
  driver->EPD_Clear();
  driver->EPD_DisplayPartBaseImage();
  driver->EPD_Init_Partial(); // 开启局部刷新
}

// --- 初始化 UI ---
void user_ui_init(void)
{
    // Register all apps with the manager
    app_manager.register_apps(app_list, APP_COUNT);
    
    // Create bottom bar first
    bottom_bar.create();
    
    // Initialize battery display
    int battery_level = board_div.read_battery_percentage();
    bottom_bar.update_battery(battery_level);
    last_battery_update = millis();
    
    // Start with main menu
    app_manager.switch_to_app(0);
}

// --- 循环按键检测和电池更新 ---
void reader_loop_handle(void) {
    // Update battery periodically
    if (millis() - last_battery_update > BATTERY_UPDATE_INTERVAL) {
        int battery_level = board_div.read_battery_percentage();
        bottom_bar.update_battery(battery_level);
        last_battery_update = millis();
    }
    
    // Update bottom bar with current app info
    bottom_bar.update_app_info(app_manager.get_current_app_info());
    
    // Call current app's loop
    app_manager.loop();
}