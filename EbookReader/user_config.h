#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "driver/gpio.h" // 确保引入GPIO定义

// --- 屏幕参数 ---
#define EPD_WIDTH  200
#define EPD_HEIGHT 200

// --- SPI 引脚 ---
#define EPD_SPI_NUM   SPI2_HOST
#define EPD_DC_PIN    GPIO_NUM_10
#define EPD_CS_PIN    GPIO_NUM_11
#define EPD_SCK_PIN   GPIO_NUM_12
#define EPD_MOSI_PIN  GPIO_NUM_13
#define EPD_RST_PIN   GPIO_NUM_9
#define EPD_BUSY_PIN  GPIO_NUM_8

// --- SD 卡引脚 (ESP32-S3 默认) ---
// 注意：如果你的板子 SD 卡使用了其他引脚，请修改这里
#define SD_SCK_PIN    GPIO_NUM_39
#define SD_MISO_PIN   GPIO_NUM_40
#define SD_MOSI_PIN   GPIO_NUM_41
#define SD_CS_PIN     GPIO_NUM_NC // 如果使用 MMC 模式或者是 NC，设为 -1 或 NC

// --- 电源管理 ---
#define EPD_PWR_PIN     GPIO_NUM_6
#define Audio_PWR_PIN   GPIO_NUM_42
#define VBAT_PWR_PIN    GPIO_NUM_17

// --- 按键 ---
#define BOOT_BUTTON_PIN GPIO_NUM_0
#define PWR_BUTTON_PIN  GPIO_NUM_18

// --- LVGL 配置 ---
#define EXAMPLE_LVGL_TICK_PERIOD_MS    5
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 20

#endif