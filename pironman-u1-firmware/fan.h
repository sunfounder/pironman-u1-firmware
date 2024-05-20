#pragma once

#include <Arduino.h>
#include "esp32-hal-gpio.h"
#include "esp32-hal-dac.h"
#include "esp32-hal-ledc.h"
/* ----------------------------------------------------------------
    FAN -> IO18
----------------------------------------------------------------*/
// issues : shttps://github.com/adafruit/circuitpython/issues/7871

#define FAN_DAC_MODE 0     // 0, pwm; 1, DAC
#define DAC2_START_CRR 140 // lowest voltage 1.8v, 140 = 255*1.8/3.3

#define FAN_PIN 18

#define FAN_PWM_CHANNEL 4 // tim3
#define FAN_PWM_FREQ 200
#define FAN_PWM_RESOLUTION 8 // 1-16bit (8bit->0~255)

void fanInit(void);
void fanSet(uint8_t power);
void fanClose();