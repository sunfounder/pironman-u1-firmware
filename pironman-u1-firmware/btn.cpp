#include "btn.h"

void btnInit()
{
    // pinMode(BTN_PIN, INPUT);
    pinMode(BTN_PIN, INPUT_PULLUP);
}

uint8_t btnRead()
{
    // return digitalRead(BTN_PIN);
    return !digitalRead(BTN_PIN); // reverse
}

// key state scan & key events
// =================================================================
#define KeyScanInterval 10 // ms Key Scan Interval

#define LongPressTime2S 2000   // ms
#define LongPressTime3S 3000   // ms
#define LongPressTime5S 5000   // ms
#define DoulePressInterval 200 // ms

enum KeyMachineState keyMachineState;
enum KeyState keyState;

uint8_t hasPressed = 0;
uint16_t keyScanTimeCnt = 0;
uint16_t longPressTimeCnt = 0;
uint16_t doulePressTimeCnt = 0;

void keyStateHandler()
{
    if (keyScanTimeCnt >= KeyScanInterval)
    {
        // -------------- key detect --------------
        switch (keyMachineState)
        {
        case BTN_UP:
            if (BTN) // pressed
            {
                keyMachineState = BTN_DOWN_SHAKE;
            }
            else
            {
                if (hasPressed)
                {
                    if (doulePressTimeCnt > DoulePressInterval) // 单击：按下松开后200ms
                    {
                        keyState = SingleClicked;
                        hasPressed = 0;
                    }
                }
                else
                {
                    keyState = Released; // 松开
                }
            }
            break;
        case BTN_DOWN_SHAKE:
            if (BTN)
            {
                keyMachineState = BTN_DOWN;
                longPressTimeCnt = 0; // 按下计时器复位
            }
            else
            {
                keyMachineState = BTN_UP;
            }
            break;
        case BTN_DOWN:
            if (!BTN)
            {
                keyMachineState = BTN_UP_SHAKE;
            }
            else
            {
                if (longPressTimeCnt > LongPressTime5S)
                {
                    keyState = LongPressed5S;
                }
                else if (longPressTimeCnt > LongPressTime2S)
                {
                    keyState = LongPressed2S;
                }
            }
            break;
        case BTN_UP_SHAKE:
            if (BTN)
            { // 如果为按下，视为抖动，回到上一状态
                keyMachineState = BTN_DOWN;
            }
            else
            {
                keyMachineState = BTN_UP;

                // 按下到松开
                if (keyState == LongPressed2S)
                {
                    keyState = LongPressed2SAndReleased;
                    hasPressed = 0;
                }
                else if (keyState == LongPressed5S)
                {
                    keyState = LongPressed5SAndReleased;
                    hasPressed = 0;
                }
                else
                {
                    if (hasPressed)
                    {
                        keyState = DouleClicked;
                        hasPressed = 0;
                    }
                    else
                    {
                        hasPressed = 1;
                        doulePressTimeCnt = 0;
                    }
                }
                longPressTimeCnt = 0;
            }
            break;
        default:
            keyMachineState = BTN_UP;
            break;
        }
        // 检测间隔计数清零
        keyScanTimeCnt = 0;
    }
}