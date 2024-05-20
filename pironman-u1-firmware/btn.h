#pragma once

#include "esp32-hal-gpio.h"
/*----------------------------------------------------------------
    BTN -> IO21
----------------------------------------------------------------*/
#define BTN_PIN 21

enum KeyMachineState // state machine enum
{
    BTN_UP = 0,         // key released
    BTN_DOWN_SHAKE = 1, // key press shake
    BTN_DOWN = 2,       // key pressed
    BTN_UP_SHAKE = 3,   // key release shake
};
extern enum KeyMachineState keyMachineState;

enum KeyState // state enum
{
    Released = 0,                 // 按键松开
    SingleClicked = 1,            // 单击, 按下松开100ms（双击间隔）后未按下
    DouleClicked = 2,             // 双击， 按下松开100ms（双击间隔）内按下再松开
    LongPressed2S = 3,            // 长按2s， 按下2s后未松开
    LongPressed2SAndReleased = 4, // 长按2s， 按下2s后松开
    LongPressed5S = 5,            // 长按5s， 按下5s后未松开
    LongPressed5SAndReleased = 6, // 长按5s， 按下5s后松开
};
extern enum KeyState keyState;

extern uint16_t keyScanTimeCnt;
extern uint16_t longPressTimeCnt;
extern uint16_t doulePressTimeCnt;

#define BTN btnRead()

void btnInit();
uint8_t btnRead();

void keyStateHandler(void);
void keyStateReset();
