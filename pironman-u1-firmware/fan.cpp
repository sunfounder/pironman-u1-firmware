#include "fan.h"

void fanInit(void)
{
#if FAN_DAC_MODE
    pinMode(FAN_PIN, OUTPUT);
#else
    pinMode(FAN_PIN, OUTPUT);
    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
    ledcAttachPin(FAN_PIN, FAN_PWM_CHANNEL);
    ledcWrite(FAN_PWM_CHANNEL, 0);
#endif
}

void fanSet(uint8_t power)
{
    uint8_t crr = 0;
    if (power > 100)
    {
        power = 100;
    }

#if FAN_DAC_MODE

    if (power == 0)
    {
        fanClose();
    }
    else
    {
        crr = DAC2_START_CRR + (uint8_t)((255 - DAC2_START_CRR) * power / 100);
        dacWrite(FAN_PIN, crr);
    }

#else

    if (power != 0)
    {
        power += 20;
    }
    crr = (uint8_t)(255 * power / 120);
    ledcWrite(FAN_PWM_CHANNEL, crr);
#endif
}

void fanClose()
{
#if FAN_DAC_MODE
    pinMode(FAN_PIN, OUTPUT);
    dacDisable(FAN_PIN);
    digitalWrite(FAN_PIN, LOW);
    // dacWrite(FAN_PIN, 0);
#else
    ledcWrite(FAN_PWM_CHANNEL, 0);
#endif
}