#ifndef USER_APP_H
#define USER_APP_H

#include "src/display/epaper_driver_bsp.h"
#include "lvgl.h"

// 声明外部驱动对象
extern epaper_driver_display *driver;

#ifdef __cplusplus
extern "C" {
#endif

// 初始化硬件
void user_app_init(void);

// 初始化 UI
void user_ui_init(void);

// 按键处理 (在 loop 中调用)
void reader_loop_handle(void);

#ifdef __cplusplus
}
#endif

#endif