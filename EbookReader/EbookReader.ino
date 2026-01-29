#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "user_config.h"
#include "lvgl.h"
#include "user_app.h"
#include <Arduino.h>

/* --- LVGL 移植部分 (保留你的原始逻辑) --- */
static const char *TAG = "main";
static SemaphoreHandle_t lvgl_mux = NULL;
#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define BUFF_SIZE (EPD_WIDTH * EPD_HEIGHT * BYTES_PER_PIXEL)

static bool example_lvgl_lock(int timeout_ms);
static void example_lvgl_unlock(void);
static void example_lvgl_port_task(void *arg);

// 刷新回调
static void example_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p)
{
  // 如果 driver 为空，防止崩溃
  if (!driver) {
     lv_disp_flush_ready(disp);
     return;
  }

  uint16_t *buffer = (uint16_t *)color_p;
  // 注意：不清屏，直接覆盖，提高速度
  // driver->EPD_Clear(); 
  
  for(int y = area->y1; y <= area->y2; y++) 
  {
   	for(int x = area->x1; x <= area->x2; x++)
   	{
      // 颜色转换：RGB565 转 黑白
   	  uint8_t color = (*buffer < 0x7fff) ? DRIVER_COLOR_BLACK : DRIVER_COLOR_WHITE;
   	  driver->EPD_DrawColorPixel(x,y,color);
   	  buffer++;
   	}
  }
  
  // 局部刷新提交
  driver->EPD_DisplayPart();
  lv_disp_flush_ready(disp);
}

static void example_increase_lvgl_tick(void *arg)
{
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

void lvgl_port(void)
{
  lv_init();
  lv_display_t * disp = lv_display_create(EPD_WIDTH, EPD_HEIGHT);
  lv_display_set_flush_cb(disp, example_lvgl_flush_cb);
  
  // 使用 PSRAM 分配 LVGL 显存
  uint8_t *buffer_1 = (uint8_t *)heap_caps_malloc(BUFF_SIZE, MALLOC_CAP_SPIRAM);
  assert(buffer_1);
  lv_display_set_buffers(disp, buffer_1, NULL, BUFF_SIZE, LV_DISPLAY_RENDER_MODE_FULL);
  
  // 定时器
  esp_timer_create_args_t lvgl_tick_timer_args = {};
  lvgl_tick_timer_args.callback = &example_increase_lvgl_tick;
  lvgl_tick_timer_args.name = "lvgl_tick";
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

  lvgl_mux = xSemaphoreCreateMutex();
  // 创建 LVGL 任务
  xTaskCreatePinnedToCore(example_lvgl_port_task, "LVGL", 8 * 1024, NULL, 4, NULL, 1);
  
  // 锁定并初始化 UI
  if(example_lvgl_lock(-1))
  {
    user_ui_init(); // 构建阅读器界面
    example_lvgl_unlock();
  }
}

static bool example_lvgl_lock(int timeout_ms)
{
  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;       
}

static void example_lvgl_unlock(void)
{
  xSemaphoreGive(lvgl_mux);
}

static void example_lvgl_port_task(void *arg)
{
  uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
  while(1)
  {
    if (example_lvgl_lock(-1)) 
    {
      task_delay_ms = lv_timer_handler();
      example_lvgl_unlock();
    }
    if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
    else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
    
    vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
  }
}

void setup()
{
  // 1. 初始化用户硬件 (SD, 屏幕驱动, IO)
  user_app_init();
  
  // 2. 启动 LVGL 线程
  lvgl_port(); 
}

void loop() 
{
  // 在主循环处理按键逻辑
  reader_loop_handle();
  
  // 延时让出资源
  delay(20);
}