#pragma once

#include "esp32-hal-gpio.h"
#include "esp32-hal-ledc.h"
#include "esp32-hal-dac.h"
/* ----------------------------------------------------------------
    FAN -> IO18
----------------------------------------------------------------*/
#define FAN_PWM_FREQ 100
#define FAN_PWM_RESOLUTION 8 // 1-16bit (8bit->0~255)
#define FAN_PIN 18
#define FAN_PWM_CHANNEL 3

void fanInit(void);
void fanSet(uint8_t power);
void fanClose();