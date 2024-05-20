#pragma once

#include <Arduino.h>
#include "esp32-hal-ledc.h"
#include "debug.h"
/* ----------------------------------------------------------------
    LED_R -> IO26
    LED_G -> IO33
    LED_B -> IO38
----------------------------------------------------------------*/
#define LED_PWM_FREQ 10000
#define LED_PWM_RESOLUTION 8 // 1-16bit (8bit->0~255)

#define LED_R_PIN 38
#define LED_G_PIN 33
#define LED_B_PIN 26

#define LED_R_PWM_CHANNEL 0
#define LED_G_PWM_CHANNEL 1
#define LED_B_PWM_CHANNEL 2

// ---- rgb mode ---
extern uint8_t rgbMode;
extern uint16_t rgbTimeCnt;

#define RGB_MODE_OFF 0           // LED OFF
#define RGB_MODE_CHARGING 1      // charging
#define RGB_MODE_USB_POWERED 2   // usb powered
#define RGB_MODE_BAT_POWERED 3   // battery powered
#define RGB_MODE_LOW_BATTERY 4   // low battery
#define RGB_MODE_WAIT_SHUTDOWN 5 // waiting for shutdown

// min interval 50
#define RGB_BREATH_INTERVAL 100 // ms
#define RGB_BLINK_INTERVAL 500
#define RGB_STATIC_INTERVAL 3000

#define CHARGING_COLOR 255, 50, 0 // yellow
#define CHARGING_COLOR_R 255
#define CHARGING_COLOR_G 50
#define CHARGING_COLOR_B 0
#define USB_POWERED_COLOR 0, 255, 0     // green
#define BAT_POWERED_COLOR 255, 200, 0   // orange
#define LOW_BATTERY_COLOR 255, 0, 0     // red
#define WAIT_SHUTDOWN_COLOR 255, 0, 255 // purple

// --- functions ---
void rgbLEDInit(void);
void rgbWrite(uint8_t r, uint8_t g, uint8_t b);
void rgbClose();
void rgbHandler();
