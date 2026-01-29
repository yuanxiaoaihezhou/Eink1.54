#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "board_power_bsp.h"

#define BATTERY_ADC_CHANNEL ADC1_CHANNEL_0  // GPIO1 (adjust if needed)

board_power_bsp_t::board_power_bsp_t(uint8_t _epd_power_pin,uint8_t _audio_power_pin,uint8_t _vbat_power_pin) :
    epd_power_pin(_epd_power_pin),
    audio_power_pin(_audio_power_pin),
    vbat_power_pin(_vbat_power_pin) {
    gpio_config_t gpio_conf = {};                                                            
        gpio_conf.intr_type = GPIO_INTR_DISABLE;                                             
        gpio_conf.mode = GPIO_MODE_OUTPUT;                                                   
        gpio_conf.pin_bit_mask = (0x1ULL << epd_power_pin) | (0x1ULL << audio_power_pin) | (0x1ULL << vbat_power_pin);
        gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;                                      
        gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;                                           
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));  
}

board_power_bsp_t::~board_power_bsp_t() {

}

void board_power_bsp_t::POWEER_EPD_ON() {
    gpio_set_level((gpio_num_t)epd_power_pin,0);
}

void board_power_bsp_t::POWEER_EPD_OFF() {
    gpio_set_level((gpio_num_t)epd_power_pin,1);
}

void board_power_bsp_t::POWEER_Audio_ON() {
    gpio_set_level((gpio_num_t)audio_power_pin,0);
}

void board_power_bsp_t::POWEER_Audio_OFF() {
    gpio_set_level((gpio_num_t)audio_power_pin,1);
}

void board_power_bsp_t::VBAT_POWER_ON() {
    gpio_set_level((gpio_num_t)vbat_power_pin,1);
}

void board_power_bsp_t::VBAT_POWER_OFF() {
    gpio_set_level((gpio_num_t)vbat_power_pin,0);
}

int board_power_bsp_t::read_battery_percentage() {
    // For now, return a default value
    // The actual battery reading would depend on hardware configuration
    // This can be updated based on the specific board's battery monitoring circuit
    
    // Enable VBAT power for measurement
    VBAT_POWER_ON();
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait for stabilization
    
    // Try to configure and read ADC
    // Note: This may need adjustment based on actual hardware
    int percentage = 85; // Default value
    
    #ifdef CONFIG_IDF_TARGET_ESP32S3
    // ESP32-S3 ADC reading code would go here if hardware supports it
    // For now using default value
    #else
    // ESP32 ADC reading
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_11);
    
    // Read ADC value (average of 10 samples)
    uint32_t adc_reading = 0;
    for (int i = 0; i < 10; i++) {
        adc_reading += adc1_get_raw(BATTERY_ADC_CHANNEL);
    }
    adc_reading /= 10;
    
    // Convert to voltage (in mV)
    uint32_t voltage = (adc_reading * 2600) / 4095;
    voltage *= 2; // Assuming 1:2 voltage divider
    
    // Calculate percentage based on typical Li-Po voltage range
    if (voltage >= 4200) {
        percentage = 100;
    } else if (voltage <= 3000) {
        percentage = 0;
    } else {
        percentage = ((voltage - 3000) * 100) / (4200 - 3000);
    }
    #endif
    
    return percentage;
}

