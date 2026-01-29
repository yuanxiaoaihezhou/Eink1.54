#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "user_app.h"
#include "user_config.h"
#include "src/power/board_power_bsp.h"

// --- 全局对象 ---
epaper_driver_display *driver = NULL;
board_power_bsp_t board_div(EPD_PWR_PIN, Audio_PWR_PIN, VBAT_PWR_PIN);

// --- 声明字体 ---
// 必须与你生成的 .c 文件内最后的结构体名称一致
LV_FONT_DECLARE(my_font_chinese_16); 

// --- 变量 ---
lv_obj_t * label_content;
lv_obj_t * label_status;

File bookFile;
const char* BOOK_PATH = "/book.txt"; // 请确保SD卡根目录有此文件
unsigned long currentOffset = 0;
int pageNum = 1;

// N8R8 有 8MB PSRAM，我们可以用大一点的缓存
// 每次读取 512 字节用于显示
const int BUFFER_SIZE = 512; 
char textBuffer[BUFFER_SIZE + 1]; 

// --- 内部函数声明 ---
void load_page(unsigned long offset);
void show_error(const char* msg);

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

// --- 安全读取 UTF-8 文本 ---
// 防止在缓冲区末尾切断中文字符
int read_utf8_safe(File &f, char* buf, int maxLen) {
    int readLen = f.readBytes(buf, maxLen);
    if (readLen <= 0) return 0;

    // 检查最后一个字节
    // 如果是 UTF-8 多字节字符的中间部分 (最高两位是 10xxxxxx)
    // 或者开头部分但未结束
    int safeLen = readLen;
    
    // 如果最后一个字节是 10xxxxxx，说明被切断了，回退
    while (safeLen > 0 && (buf[safeLen-1] & 0xC0) == 0x80) {
        safeLen--;
    }

    // 此时 buf[safeLen-1] 可能是单字节字符(0xxxxxxx) 或 多字节开头(11xxxxxx)
    // 如果是多字节开头，我们要检查后面是否跟了足够的字节
    if (safeLen > 0) {
        unsigned char lastChar = (unsigned char)buf[safeLen-1];
        int needed = 0;
        if ((lastChar & 0xE0) == 0xE0) needed = 3; // 3字节字符 (中文通常在这里)
        else if ((lastChar & 0xC0) == 0xC0) needed = 2; // 2字节字符
        
        // 如果剩余空间不足以存放完整字符 (实际上我们已经把后面的砍掉了，所以肯定不足)
        // 这里的逻辑简化处理：只要末尾是多字节的开头，说明这个字没读完（因为上面循环已经把后续字节去掉了）
        // 除非这个字正好读完了。
        // 为了简单：如果最后一个字节是多字节开头，直接砍掉，留给下一页
        if ((lastChar & 0xC0) == 0xC0) {
            safeLen--; 
        }
    }

    // 修正文件指针位置，以便下次从正确位置读取
    int backtrack = readLen - safeLen;
    if (backtrack > 0) {
        f.seek(f.position() - backtrack);
    }
    
    buf[safeLen] = '\0'; // 字符串截断
    return safeLen;
}

void load_page(unsigned long offset) {
    if (!SD.exists(BOOK_PATH)) {
        show_error("No /book.txt");
        return;
    }

    bookFile = SD.open(BOOK_PATH, FILE_READ);
    if (bookFile) {
        bookFile.seek(offset);
        
        // 安全读取
        read_utf8_safe(bookFile, textBuffer, BUFFER_SIZE - 1);
        
        // 更新 UI
        lv_label_set_text(label_content, textBuffer);
        
        // 更新页码
        char status_buf[32];
        sprintf(status_buf, "Page: %d", pageNum);
        lv_label_set_text(label_status, status_buf);
        
        Serial.printf("Page %d Loaded. Offset: %lu\n", pageNum, offset);
        
        bookFile.close();
    }
}

void show_error(const char* msg) {
    lv_label_set_text(label_content, msg);
}

// --- 初始化 UI ---
void user_ui_init(void)
{
    // 样式配置
    static lv_style_t style_text;
    lv_style_init(&style_text);
    lv_style_set_text_font(&style_text, &my_font_chinese_16); // 使用自定义中文字体
    lv_style_set_text_color(&style_text, lv_color_black());
    lv_style_set_text_line_space(&style_text, 4); // 行间距

    // 1. 小说内容区域
    label_content = lv_label_create(lv_scr_act());
    lv_obj_add_style(label_content, &style_text, 0);
    lv_obj_set_width(label_content, 196); // 屏幕宽200，留边距
    lv_obj_align(label_content, LV_ALIGN_TOP_MID, 0, 2);
    lv_label_set_long_mode(label_content, LV_LABEL_LONG_WRAP); // 自动换行
    lv_label_set_text(label_content, "正在初始化...");

    // 2. 底部状态栏 (页码)
    label_status = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_14, 0); // 页码用默认英文
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_label_set_text(label_status, "Ready");

    // 加载第一页
    currentOffset = 0;
    pageNum = 1;
    load_page(currentOffset);
}

// --- 循环按键检测 ---
unsigned long last_key_time = 0;
// 简单的上一页逻辑：因为TXT是流式的，没法直接知道上一页offset。
// 这里用数组记录简单的历史。
unsigned long history_offsets[500]; 

void reader_loop_handle(void) {
    if (millis() - last_key_time < 300) return; // 去抖

    // 下一页 (BOOT)
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        last_key_time = millis();
        
        // 记录历史
        if (pageNum < 500) history_offsets[pageNum] = currentOffset;
        
        // 估算下一页位置：
        // LVGL 并没有告诉我们到底显示了多少个字（这是复杂点）。
        // 简化做法：我们每次固定读取 BUFFER_SIZE 个字节给 LVGL。
        // 虽然 LVGL 可能因为 wrap 没显示完，或者显示空了。
        // 完美的电子书需要计算排版，但对于简易阅读器，我们假设一页大约前进 300-400 字节。
        // 这里简单粗暴：直接 currentOffset += 300;
        // 更好的方法：利用 TXT 换行符统计，但中文排版很难精准。
        
        // 简易方案：每次前进 350 字节（根据 1.54寸屏幕 字号16 估算）
        currentOffset += 350; 
        pageNum++;
        load_page(currentOffset);
    }

    // 上一页 (PWR)
    if (digitalRead(PWR_BUTTON_PIN) == LOW) {
        last_key_time = millis();
        if (pageNum > 1) {
            pageNum--;
            // 如果有历史记录，读取历史；否则倒退 350
            if (pageNum < 500 && history_offsets[pageNum] > 0) {
                 currentOffset = history_offsets[pageNum];
            } else {
                 if (currentOffset > 350) currentOffset -= 350;
                 else currentOffset = 0;
            }
            load_page(currentOffset);
        }
    }
}