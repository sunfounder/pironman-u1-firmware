#include "rgb_led.h"

void rgbLEDInit(void)
{
    // uint32_t    ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
    // void        ledcAttachPin(uint8_t pin, uint8_t channel);

    ledcSetup(LED_R_PWM_CHANNEL, LED_PWM_FREQ, LED_PWM_RESOLUTION);
    ledcSetup(LED_G_PWM_CHANNEL, LED_PWM_FREQ, LED_PWM_RESOLUTION);
    ledcSetup(LED_B_PWM_CHANNEL, LED_PWM_FREQ, LED_PWM_RESOLUTION);
    ledcAttachPin(LED_R_PIN, LED_R_PWM_CHANNEL);
    ledcAttachPin(LED_G_PIN, LED_G_PWM_CHANNEL);
    ledcAttachPin(LED_B_PIN, LED_B_PWM_CHANNEL);
}

void rgbWrite(uint8_t r, uint8_t g, uint8_t b)
{
    // ledcWrite(uint8_t channel, uint32_t duty);
    ledcWrite(LED_R_PWM_CHANNEL, r);
    ledcWrite(LED_G_PWM_CHANNEL, g);
    ledcWrite(LED_B_PWM_CHANNEL, b);
}

void rgbClose()
{
    rgbWrite(0, 0, 0);
}

// rgbHandler
// =================================================================
uint16_t rgbTimeCnt = 0;
uint8_t rgbMode = 0;
uint8_t lastrgbMode = 0;
float rgbBrightness = 0;
uint8_t rgbBlinkFlag = 0;

void rgbHandler()
{
    uint8_t r, g, b;

    if (lastrgbMode != rgbMode)
    {
        lastrgbMode = rgbMode;
        rgbTimeCnt = 0xffff;
    }

    switch (rgbMode)
    {
    case RGB_MODE_OFF:
        if (rgbTimeCnt > RGB_STATIC_INTERVAL)
        {
            rgbClose();
            rgbTimeCnt = 0;
        }
        break;
    case RGB_MODE_CHARGING:
        if (rgbTimeCnt > RGB_BREATH_INTERVAL)
        {
            if (rgbBrightness == 1)
            {
                rgbBrightness = 0;
            }
            else
            {
                rgbBrightness += 0.05;
            }
            r = (uint8_t)(rgbBrightness * (float)CHARGING_COLOR_R);
            g = (uint8_t)(rgbBrightness * (float)CHARGING_COLOR_G);
            b = (uint8_t)(rgbBrightness * (float)CHARGING_COLOR_B);
            rgbWrite(r, g, b);
            rgbTimeCnt = 0;
        }
        break;
    case RGB_MODE_USB_POWERED:
        if (rgbTimeCnt > RGB_STATIC_INTERVAL)
        {
            rgbWrite(USB_POWERED_COLOR);
            rgbTimeCnt = 0;
        }
        break;
    case RGB_MODE_BAT_POWERED:
        if (rgbTimeCnt > RGB_STATIC_INTERVAL)
        {
            rgbWrite(BAT_POWERED_COLOR);
            rgbTimeCnt = 0;
        }
        break;
    case RGB_MODE_LOW_BATTERY:
        if (rgbTimeCnt > RGB_BLINK_INTERVAL)
        {
            rgbBlinkFlag = !rgbBlinkFlag;
            if (rgbBlinkFlag)
            {
                rgbWrite(LOW_BATTERY_COLOR);
            }
            else
            {
                rgbClose();
            }
            rgbTimeCnt = 0;
        }
        break;
    case RGB_MODE_WAIT_SHUTDOWN:
        if (rgbTimeCnt > RGB_BLINK_INTERVAL)
        {
            rgbBlinkFlag = !rgbBlinkFlag;
            if (rgbBlinkFlag)
            {
                rgbWrite(WAIT_SHUTDOWN_COLOR);
            }
            else
            {
                rgbClose();
            }
            rgbTimeCnt = 0;
        }
        break;
    default:
        break;
    }
}
