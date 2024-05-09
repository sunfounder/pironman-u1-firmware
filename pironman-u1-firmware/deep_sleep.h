#pragma once

// #define CONFIG_IDF_TARGET_ESP32 0
#include "Arduino.h"
//~\Arduino15\packages\esp32\hardware\esp32\2.0.14\tools\sdk\esp32s2\include\esp_hw_support\include\esp_sleep.h
#include "esp_sleep.h"
#include "debug.h"
// #if CONFIG_IDF_TARGET_ESP32S2
// typedef enum
// {
//     ESP_EXT1_WAKEUP_ANY_LOW = 0,  //!< Wake the chip when any of the selected GPIOs go low
//     ESP_EXT1_WAKEUP_ANY_HIGH = 1, //!< Wake the chip when any of the selected GPIOs go high
//     ESP_EXT1_WAKEUP_ALL_LOW __attribute__((deprecated("wakeup mode \"ALL_LOW\" is no longer supported after ESP32, \
//         please use ESP_EXT1_WAKEUP_ANY_LOW instead"))) = ESP_EXT1_WAKEUP_ANY_LOW
// } esp_sleep_ext1_wakeup_mode_t;
// #endif
/* ----------------------------------------------------------------
    BTN -> IO21
    VBUS_3V3 -> IO6

Note: Only RTC GPIO can be used as a source for external wake
source.

ESP32_S2 RTC_GPIO : GPIO0 ~ GPIO21
---------------------------------------------------------------- */

// #define BUTTON_PIN_BITMASK 0x200040 // 2^6 + 2^21
// #define BUTTON_PIN_BITMASK 0x40 // 2^6
#define BUTTON_PIN_BITMASK 0x200000 // 2^21

extern uint32_t bootCount;
extern esp_sleep_wakeup_cause_t wakeup_reason;

void print_wakeup_reason();
void print_GPIO_wake_up();
void enterDeepSleep();
