#include "fan.h"

void fanInit(void)
{
    // ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
    // ledcAttachPin(FAN_PIN, FAN_PWM_CHANNEL);

    pinMode(FAN_PIN, OUTPUT);
}

void fanSet(uint8_t power)
{
    uint8_t pluseWidth = 0;

    if (power > 100)
    {
        power = 100;
    }
    pluseWidth = (uint8_t)(255 * power / 100);
    // ledcWrite(FAN_PWM_CHANNEL, pluseWidth);

    dacWrite(FAN_PIN, 0);
}

void fanClose()
{
    ledcWrite(FAN_PWM_CHANNEL, 0);
}