#include "deep_sleep.h"

RTC_DATA_ATTR uint32_t bootCount = 0;
esp_sleep_wakeup_cause_t wakeup_reason;

void print_wakeup_reason()
{
    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        info("Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        info("Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        info("Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        info("Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        info("Wakeup caused by ULP program");
        break;
    default:
        info("Wakeup was not caused by deep sleep: %d", wakeup_reason);
        break;
    }
}

void enterDeepSleep()
{
    esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);
    // esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
    info("Deep sleep now");
    esp_deep_sleep_start();
}

void print_GPIO_wake_up()
{
    uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
    if (GPIO_reason)
    {
        info("GPIO that triggered the wake up: GPIO %d", (log(GPIO_reason)) / log(2));
    }
    else
    {
        info("GPIO that triggered the wake up: Null");
    }
}
