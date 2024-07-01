#pragma once

#include <Arduino.h>
#include "esp32-hal-gpio.h"
#include "esp32-hal-adc.h"
#include "btn.h"
#include <Preferences.h>
#include "debug.h"
/* ----------------------------------------------------------------
DAC:
    CHG_CRNT_CTRL -> IO17

INPUT:
    DEFAULT_ON -> IO39
    BTN -> IO21
    #VBS_DT -> IO6

OUTPUT:
    BAT_EN -> IO11
    DC_EN -> IO15
    USB_EN -> IO16
    PI5_BTN -> IO41

ADC:
    CHG_3V3 -> IO1
    BATTERY_CURRENT_REF -> IO2
    BATTERY_CURRENT_FILTERED -> IO3
    USB_CURRENT_FILTERED -> IO4
    VBUS_3V3 -> IO5
    OUTPUT_CURRENT_FILTERED -> IO7
    VOUT_3V3 -> IO8
    BT_LV_3V3 -> IO9

----------------------------------------------------------------*/
#define ADC_RAW_VOLT 0 // for debug: to show raw voltage (no gain)

// --- define pins ---
#define DEFAULT_ON_PIN 39
#define POWER_SOURCE_PIN 42

#define DC_EN_PIN 10
#define BAT_EN_PIN 11
#define USB_EN_PIN 12
#define PI5_BTN_PIN 41

#define CHG_3V3_PIN 1
#define BAT_CURRENT_REF_PIN 2
#define BAT_CURRENT_PIN 3
#define USB_CURRENT_PIN 4
#define VBUS_3V3_PIN 5
#define OUTPUT_CURRENT_PIN 7
#define BAT_LV_3V3_PIN 9
#define VOUT_3V3_PIN 8

#define CHG_CRNT_CTRL_PIN 17

// --- adc voltage divider gain ---
#define CHG_3V3_GAIN 2.0
#define POWER_SOURCE_GAIN 3.0
#define VBUS_3V3_GAIN 2.0
#define BAT_LV_3V3_GAIN 3.439
#define VOUT_3V3_GAIN 2.0

#define BAT_CURRENT_GAIN 2.0
#define USB_CURRENT_GAIN 2.0
#define OUTPUT_CURRENT_GAIN 2.0

// --- global variables ---
#define USB_MIN_VOLTAGE 2500 // mv, whether usb is plugged in or not

#define BAT_MIN_VOLTAGE 5800          // mv, whether bat is plugged in or not
#define BAT_MAX_VOLTAGE 8450          // mv, Exceeding the normal maximum voltage is considered as abnormal voltage (battery unplugged)
#define BAT_CAPACITY_MAX_VOLTAGE 8240 // mv, voltage at 100% battery capacity
#define BAT_CAPACITY_MIN_VOLTAGE 6500 // mv, voltage at 0% battery capacity
#define P7Voltage 6800
#define MAX_CAPACITY 2000

#define BAT_IS_CHG_MIN_CRNT 120   // mA, whether the battery is charging
#define BAT_IS_POWERD_MIN_CRNT 50 // mA, whether the battery is powered

#define RPI_NEED_MIN_CRNT 250 // mA, whether the rpi is On

#define DEFAULT_BATTERY_IR 0.30 // Default battery internal resistance 0.3 Ohm

#define DEFAULT_ON (bool)checkDefaultOn()
#define POWER_SOURCE (bool)checkPowerSource()

extern uint8_t outputState;
extern bool powerSource;

extern uint16_t chg3V3Volt;
extern uint16_t batCrntRefVolt;
extern uint16_t usbVolt;
extern uint16_t batVolt;
extern uint16_t outputVolt;

extern int16_t batCrnt;
extern uint16_t usbCrnt;
extern uint16_t outputCrnt;

extern bool isBatPlugged;
extern bool isBatPowered;
extern uint8_t batPct;
extern float batCapacity;

extern bool isUsbPlugged;
extern bool isCharging;
extern bool isLowBattery;

// --- average ---
#define AVERAGE_FILTER_SIZE 20

extern uint8_t avgFilterIndex;

extern uint16_t usbVoltFilterBuf[];
extern uint16_t usbCrntFilterBuf[];
extern uint16_t batVoltFilterBuf[];
extern int16_t batCrntFilterBuf[];
extern uint16_t outputVoltFilterBuf[];
extern uint16_t outputCrntFilterBuf[];

extern uint32_t usbVoltFilterSum;
extern uint32_t usbCrntFilterSum;
extern uint32_t batVoltFilterSum;
extern int32_t batCrntFilterSum;
extern uint32_t outputVoltFilterSum;
extern uint32_t outputCrntFilterSum;

extern uint16_t usbVoltAvg;
extern uint16_t usbCrntAvg;
extern uint16_t batVoltAvg;
extern int16_t batCrntAvg;
extern uint16_t outputVoltAvg;
extern uint16_t outputCrntAvg;

// --- functions ---
// void powerIoInit(void);
// void powerSourcePinInit();
bool checkDefaultOn();
bool checkPowerSouce();
void powerOutOpen();
void powerOutClose();
void pi5BtnSingleClick();
void pi5BtnDoubleClick();
void batEN(bool sw);
void dcEN(bool sw);
void usbEN(bool sw);

void requestShutDown(uint8_t code);

uint16_t readADCVolt(uint8_t pin);
uint16_t readChg3V3Volt(void);
uint16_t readBatCrntRefVolt();
uint16_t readUsbVolt();
uint16_t readBatVolt();
uint16_t readOutputVolt();
int16_t readBatCrnt();
uint16_t readUsbCrnt();
uint16_t readOutputCrnt();
void powerDataFilter();

void chgCrntCtrlInit(void);
void chgCrntCtrl(uint16_t volt);
float pidCalculate(uint16_t current_vol);
void chargeAdjust(bool _powerSourceVolt, uint16_t _vbusVolt);

void batIRInit();
void updateBatIR();
void saveBatIR();

void batCapacityInit();
void updateBatCapacity();
uint8_t batVolt2Pct(uint16_t volt);
uint8_t batCapacity2Pct(uint16_t capacity);

int round2Int(float num);