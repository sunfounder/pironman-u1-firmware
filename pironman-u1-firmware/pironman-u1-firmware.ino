/********************************************************************************
                  pironman-u1-firmware (ESP32-S2)
https://github.com/sunfounder/pironman-u1-firmware

CORE:
    ESP32-S2
    Flash: 4MB
    SRAM: 320 KB
    RTC SRAM: 16 KB
    partitions: (Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS))
        - nvs: 20 KB
        - app0: 1,920 KB
        - app1: 1,920 KB
        - spiffs: 128 KB

IIC:
    SDA_1 -> IO13
    SCL_1 -> IO14

SD SPI:
    MOSI -> IO35
    MISO -> IO37
    SCLK -> IO36
    CS0 -> IO34

SD_DT -> IO40

USB:
    D+ -> USB_D+
    D- -> USB_D-

PWM:
    RGB_LED:
        LED_R -> IO38
        LED_G -> IO33
        LED_B -> IO26

    FAN:
        FAN -> IO18

DAC:
    CHG_CRNT_CTRL -> IO17

INPUT:
    DEFAULT_ON -> IO39
    BTN -> IO21
    Power_Source_1V8 -> IO42
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

    VBUS_3V3 -> IO6
    OUTPUT_CURRENT_FILTERED -> IO7
    BT_LV_3V3 -> IO8
    VOUT_3V3 -> IO9

*******************************************************************************/
#include "Esp.h"
#include "debug.h"
#include "Wire.h"
#include "wifi_helper.h"
#include "webpage.h"
#include <ArduinoJson.h>
#include "power.h"
#include "rgb_led.h"
#include "fan.h"
#include "btn.h"
#include "sdcard.h"
#include "file_system.h"
#include "mqtt.h"
#include "date_time.h"
#include "config.h"

// VERSION INFO
// =================================================================
#define VERSION "0.0.8"
#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_MICRO 8

#define BOARD_ID 0x00

// global variables
// =================================================================
WiFiHelper wifi = WiFiHelper();

// Functions
// =================================================================
void saveDataIntoSD();
void shutdown();
void rpiStateHandler();

// -----------------
void wifiStart()
{
    Serial.println("\nWiFi start ...");

    // ap
    wifi.connectAP(config.apSSID, config.apPSK);

    // sta
    if (config.staENABLE)
    {
        wifi.connectSTA(config.staSSID, config.staPSK);
    }
}

// mqtt
// =================================================================
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("MQTT Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

// data buffer
// =================================================================
// ---------------data----------------------
#define maxDataBufferLen 255
uint8_t dataBuffer[maxDataBufferLen] = {0};

// ---------------setting----------------------
#define maxSettingBufferLen 12
uint8_t settingBuffer[maxSettingBufferLen] = {0};

// ---------------json----------------------
StaticJsonDocument<512> dataDoc;

// data buffer
// =================================================================
String myOnData()
{
    String jsonString;
    serializeJson(dataDoc, jsonString);
    return jsonString;
}

// I2C
// =================================================================
#define I2C_FREQ 400000
#define I2C_SDA 13
#define I2C_SCL 14
#define I2C_DEV_ADDR 0x5A

uint8_t addr = 0;

void onReceive(int numBytes)
{
    uint8_t ch = 0;
    uint8_t cmd = 0;
    uint8_t settingAddr = 0;

    debug("i2c onReceive: %d", numBytes);

    //---- read cmd ----
    cmd = Wire.read();
    // ----- set reg start addr-----
    if (cmd == 0x01)
    {
        ch = Wire.read();
        addr = ch;
    }
    // ----- setting -----
    else if (cmd == 0x02)
    {
        settingAddr = Wire.read(); // first byte is address
        for (uint8_t j = 2; j < numBytes; j++)
        {
            ch = Wire.read();
            if (settingAddr < maxSettingBufferLen)
            {
                settingBuffer[settingAddr] = ch;
                settingAddr++;
            }
        }
    }
}

void onRequest()
{
    Serial.print("onRequest: addr: ");
    Serial.print(addr);
    Serial.print(" , val: ");
    Serial.println(dataBuffer[addr]);

    Wire.write(dataBuffer[addr++]);
}

void i2cInit()
{
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Wire.begin((uint8_t)I2C_DEV_ADDR, I2C_SDA, I2C_SCL, I2C_FREQ);
}
// time counter (TIM0) 5ms interrupt,
// =================================================================
#define TIM0_COUNTER_PREIOD 5 // ms
#define TIM0_DIV 80           // 80MHz / 80 = 1MHz, count plus one every us
#define TIM0_TICKS 5000       // 5000 * us = 5ms

hw_timer_t *tim0 = NULL;
uint16_t time50Cnt = 0;
uint16_t time1000Cnt = 0;

void IRAM_ATTR onTim0()
{
    keyScanTimeCnt += TIM0_COUNTER_PREIOD;
    longPressTimeCnt += TIM0_COUNTER_PREIOD;
    doulePressTimeCnt += TIM0_COUNTER_PREIOD;
    rgbTimeCnt += TIM0_COUNTER_PREIOD;
    time50Cnt += TIM0_COUNTER_PREIOD;
    time1000Cnt += TIM0_COUNTER_PREIOD;

    // Serial.println("onTim0");
}

void tim0Init(void)
{
    tim0 = timerBegin(0, TIM0_DIV, true);
    timerAttachInterrupt(tim0, &onTim0, true);
    timerAlarmWrite(tim0, TIM0_TICKS, true);
    timerAlarmEnable(tim0);
}

// time counter (TIM3) 100ms interrupt, used for boot rgb animation
// =================================================================
#define TIM3_COUNTER_PREIOD 100 // ms
#define TIM3_DIV 80             // 80MHz / 80 = 1MHz, count plus one every us
#define TIM3_TICKS 100000       // 5000 * us = 5ms

#define BOOT_COLOR_R 0
#define BOOT_COLOR_G 0
#define BOOT_COLOR_B 255

extern float rgbBrightness;

void IRAM_ATTR onbootRgbAnimation()
{
    if (rgbBrightness >= 1)
    {
        rgbBrightness = 0.0;
    }
    else
    {
        rgbBrightness += 0.05;
    }

    uint8_t _r = (uint8_t)(rgbBrightness * (float)BOOT_COLOR_R);
    uint8_t _g = (uint8_t)(rgbBrightness * (float)BOOT_COLOR_G);
    uint8_t _b = (uint8_t)(rgbBrightness * (float)BOOT_COLOR_B);
    rgbWrite(_r, _g, _b);
}

void bootRGBAnimationStart(void)
{
    hw_timer_t *tim3 = NULL;
    tim3 = timerBegin(3, TIM3_DIV, true);
    timerAttachInterrupt(tim3, &onbootRgbAnimation, true);
    timerAlarmWrite(tim3, TIM3_TICKS, true);
    timerAlarmEnable(tim3);
}

void bootRgbAnimationStop(void)
{
    hw_timer_t *tim3 = NULL;
    tim3 = timerBegin(3, TIM3_DIV, true);
    timerAlarmDisable(tim3);
}

// power manager
// =================================================================
#define LOW_BATTERY_WARNING 20  // 20% Low battery warning（led flashing）
#define LOW_BATTERY_POWER_OFF 0 // 0% Force shutdown

extern uint8_t shutdownRequest;

void updatePowerData()
{
    readChg3V3Volt();
    readBatCrntRefVolt();
    readUsbVolt();
    readBatVolt();
    readOutputVolt();
    readBatCrnt();
    readUsbCrnt();
    readOutputCrnt();
    checkPowerSouce();
    updateBatCapacity();
    batCapacity2Pct();

    // ---- status judgment ----
    // isBatPlugged
    if (batVolt > BAT_MIN_VOLTAGE)
    {
        isBatPlugged = true;
    }
    else
    {
        isBatPlugged = false;
    }

    // isUsbPlugged
    if (usbVolt > USB_MIN_VOLTAGE)
    {
        isUsbPlugged = true;
    }
    else
    {
        isUsbPlugged = false;
    }

    // isCharging
    if (batCrnt > BAT_IS_CHG_MIN_CRNT)
    {
        isCharging = true;
    }
    else if (batCrnt < 50)
    {
        isCharging = false;
    }

    // isBatteryPowered
    if (batCrnt < -BAT_IS_POWERD_MIN_CRNT)
    {
        isBatPowered = true;
    }
    else
    {
        isBatPowered = false;
    }

    // isLowPower
    if (batPct < LOW_BATTERY_WARNING)
    {
        isLowBattery = true;
    }
    else
    {
        isLowBattery = false;
    }

    // ---- set rgb mode ----
    if (rgbMode != RGB_MODE_WAIT_SHUTDOWN)
    {
        if (isCharging)
        {
            rgbMode = RGB_MODE_CHARGING;
        }
        else if (isLowBattery)
        {
            rgbMode = RGB_MODE_LOW_BATTERY;
        }
        else
        {
            if (isBatPowered)
            {
                rgbMode = RGB_MODE_BAT_POWERED;
            }
            else
            {
                rgbMode = RGB_MODE_USB_POWERED;
            }
        }
    }

    // ---- fill data into dataBuffer ----
    dataBuffer[1] = usbVolt >> 8 & 0xFF;
    dataBuffer[0] = usbVolt & 0xFF;

    dataBuffer[3] = usbCrnt >> 8 & 0xFF;
    dataBuffer[2] = usbCrnt & 0xFF;

    dataBuffer[5] = outputVolt >> 8 & 0xFF;
    dataBuffer[4] = outputVolt & 0xFF;

    dataBuffer[7] = outputCrnt >> 8 & 0xFF;
    dataBuffer[6] = outputCrnt & 0xFF;

    dataBuffer[9] = batVolt >> 8 & 0xFF;
    dataBuffer[8] = batVolt & 0xFF;

    dataBuffer[11] = ((uint16_t)batCrnt) >> 8 & 0xFF;
    dataBuffer[10] = ((uint16_t)batCrnt) & 0xFF;

    dataBuffer[12] = batPct & 0xFF;

    dataBuffer[14] = ((uint16_t)batCapacity) >> 8 & 0xFF;
    dataBuffer[13] = ((uint16_t)batCapacity) & 0xFF;

    dataBuffer[15] = isBatPowered & 0xFF;
    dataBuffer[16] = isUsbPlugged & 0xFF;
    dataBuffer[17] = isBatPlugged & 0xFF;
    dataBuffer[18] = isCharging & 0xFF;

    dataBuffer[19] = config.fanPower & 0xFF;

    dataBuffer[20] = shutdownRequest;

    //-------------fill data into json---------------------
    dataDoc["status"] = "true";
    dataDoc["data"]["is_input_plugged_in"] = isUsbPlugged;
    dataDoc["data"]["input_voltage"] = usbVolt;
    dataDoc["data"]["input_current"] = usbCrnt;

    dataDoc["data"]["is_battery_plugged_in"] = 1;
    dataDoc["data"]["battery_percentage"] = batPct;
    dataDoc["data"]["is_charging"] = isCharging;
    dataDoc["data"]["battery_voltage"] = batVolt;
    dataDoc["data"]["battery_current"] = batCrnt;
    dataDoc["data"]["battery_capacity"] = batCapacity;

    dataDoc["data"]["output_voltage"] = outputVolt;
    dataDoc["data"]["output_current"] = outputCrnt;

    dataDoc["data"]["output_state"] = outputState;
    dataDoc["data"]["power_source"] = (uint8_t)powerSource;

    powerDataFilter();
}

void updateShutdownPct()
{
    if (config.shutdownPct != settingBuffer[9])
    {
        config.shutdownPct = settingBuffer[9];
        dataBuffer[143] = config.shutdownPct;

        configPrefs.begin(CONFIG_PREFS_NAMESPACE); // namespace
        configPrefs.putUChar(SHUTDOWN_PCT_KEYNAME, config.shutdownPct);
        configPrefs.end();
    }
}

void updateFanPower()
{
    if (config.fanPower != settingBuffer[0])
    {
        config.fanPower = settingBuffer[0];
        dataBuffer[19] = config.fanPower;

        configPrefs.begin(CONFIG_PREFS_NAMESPACE);
        configPrefs.putUChar(FAN_POWER_KEYNAME, config.fanPower);
        configPrefs.end();

        fanSet(config.fanPower);
    }
}

/* ----------- lowBatHandler -----------*/
#define LOW_BATTERY_MAX_COUNT 100 // 5000 / 50ms, Confirm low battery for 5 seconds continuously
uint16_t lowPowerCount = 0;

void lowBatHandler()
{
    // usb 插入时不处理
    if (isUsbPlugged)
    {
        lowPowerCount = 0;
    }
    else
    {
        if (batPct < config.shutdownPct)
        {
            lowPowerCount++;
            if (lowPowerCount >= LOW_BATTERY_MAX_COUNT)
            {
                if (batPct > LOW_BATTERY_POWER_OFF)
                {
                    requestShutDown(1); // Low battery shutdown request
                    rgbMode = RGB_MODE_WAIT_SHUTDOWN;
                }
                else
                {
                    lowPowerCount = 0;
                    shutdown(); // Force shutdown
                }
            }
        }
        else
        {
            lowPowerCount = 0;
        }
    }
}

/* ----------- shutdown -----------*/
void shutdown()
{
    longPressTimeCnt = 0;
    shutdownRequest = 0;
    dataBuffer[22] = 0; // reset shutdownRequest
    rgbMode = RGB_MODE_OFF;

    if (isUsbPlugged)
    {
        powerOutClose();
    }
    else
    {
        rgbClose();
        powerOutClose();
        saveBatCapacity();
        batEN(0);
    }
}

/* ----------- rpiStateHandler -----------*/
#define MAX_NO_NEED_OUTPUT_COUNT 20
uint16_t noNeedOutputCount = 0;

void rpiStateHandler()
{
    if (outputCrnt < RPI_NEED_MIN_CRNT)
    {
        noNeedOutputCount++;
        if (noNeedOutputCount > MAX_NO_NEED_OUTPUT_COUNT)
        {
            noNeedOutputCount = 0;
            shutdown();
        }
    }
    else
    {
        noNeedOutputCount = 0;
    }
}

/* ----------- UsbUnpluggedHandler -----------*/
#define MAX_USB_UNPLUGGED_COUNT 20
uint16_t usbUnpluggedCount = 0;

void UsbUnpluggedHandler()
{
    if (outputState == 0 && !isUsbPlugged)
    {
        usbUnpluggedCount++;
        if (usbUnpluggedCount > MAX_NO_NEED_OUTPUT_COUNT)
        {
            usbUnpluggedCount = 0;
            shutdown();
        }
    }
    else
    {
        usbUnpluggedCount = 0;
    }
}

/* ----------- powerManage -----------*/
void powerManage()
{
    updatePowerData();
    chargeAdjust(powerSource, usbVolt);
    lowBatHandler();
    rpiStateHandler();
    UsbUnpluggedHandler();

    if (outputState == 0)
    {
        if (isCharging)
        {
            rgbMode = RGB_MODE_CHARGING;
        }
        else
        {
            rgbMode = RGB_MODE_OFF;
        }
    }
}

// saveDataIntoSD
// =================================================================
#define HISTORY_ROOT_DIR "/history"
void saveDataIntoSD()
{
    char historyStr[256];
    char strTemp[30];
    char dir[50];
    char path[50];
    time_t timesample;
    struct tm timeinfo;

    // --- get time ---
    time(&timesample);
    getLocalTime(&timeinfo);

    // --- sprintf date ---

    // timesample, is_input_plugged_in, input_voltage, input_current,
    // is_battery_plugged_in, battery_percentage, is_charging,
    // battery_voltage, battery_current, output_voltage, output_current
#if 0
    sprintf(historyStr,
            "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            timesample,
            isUsbPlugged,
            usbVolt,
            usbCrnt,
            1,
            batPct,
            isCharging,
            batVolt,
            batCrnt,
            outputVolt,
            outputCrnt);
#else
    sprintf(historyStr,
            "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            timesample,
            isUsbPlugged,
            usbVoltAvg,
            usbCrntAvg,
            1,
            batPct,
            isCharging,
            batVoltAvg,
            batCrntAvg,
            outputVoltAvg,
            outputCrntAvg);
#endif
    // Serial.println(historyStr);

    // --- write data into SD ---
    if (hasSD)
    {
        // --- sprintf path ---
        strftime(strTemp, sizeof(strTemp), "%Y-%m", &timeinfo);
        sprintf(dir, "%s/%s", HISTORY_ROOT_DIR, strTemp);
        strftime(strTemp, sizeof(strTemp), "%Y-%m-%d.csv", &timeinfo);
        sprintf(path, "%s/%s", dir, strTemp);

        // createDir(SD, HISTORY_ROOT_DIR);
        createDir(SD, dir);
        appendFile(SD, path, historyStr);

        // appendFile(SD, "/2024-04-22/2024-04-22.csv", historyStr);
    }
}
// =================================================================
void keyEventHandler()
{
    switch (keyState)
    {
    case SingleClicked: // 单击开启输出
        keyStateReset();
        batEN(1);

        shutdownRequest = 0;
        dataBuffer[20] = 0; // reset shutdownRequest
        rgbMode = RGB_MODE_OFF;

        rgbWrite(0, 255, 0);
        delay(100);
        rgbClose();

        powerOutOpen();

        if (!isUsbPlugged)
        {
            if (batPct <= config.shutdownPct || batPct <= LOW_BATTERY_POWER_OFF)
            {
                for (int i = 0; i < 3; i++)
                {
                    rgbWrite(LOW_BATTERY_COLOR);
                    delay(RGB_BLINK_INTERVAL);
                    rgbClose();
                    delay(RGB_BLINK_INTERVAL);
                }
                rgbClose();
                longPressTimeCnt = 0;
                powerOutClose();
                batEN(0);
            }
            else
            {
                powerOutOpen();
            }
        }
        else
        {
            powerOutOpen();
        }

        break;
    case DouleClicked:
        keyStateReset();
        rgbWrite(0, 0, 255);
        delay(100);
        rgbClose();
        powerOutClose();
        pi5BtnDoubleClick();
        while (1)
        {
            if (!BTN)
            {
                delay(5);
                if (!BTN)
                {
                    delay(20);
                    longPressTimeCnt = 0;
                    break;
                }
            }
            delay(5);
        }
        break;
    case LongPressed2S:
        rgbWrite(255, 0, 255);
        break;
    case LongPressed2SAndReleased:
        keyStateReset();
        rgbClose();
        requestShutDown(2); // 2，button shutdown request
        rgbMode = RGB_MODE_WAIT_SHUTDOWN;
        break;
    case LongPressed5S:
        rgbWrite(255, 0, 0);
        rgbWrite(0, 0, 255);
        delay(100);
        rgbClose();
        powerOutClose();
        pi5BtnDoubleClick();
        while (1)
        {
            if (!BTN)
            {
                delay(5);
                if (!BTN)
                {
                    delay(20);
                    longPressTimeCnt = 0;
                    break;
                }
            }
            delay(5);
        }
        break;
    case LongPressed5SAndReleased:
        keyStateReset();
        break;
    default:
        break;
    }
}

// setup & loop
// =================================================================
void setup()
{
    uint32_t startTime;

    startTime = millis();
    Serial.begin(115200);

    // --- io init ---
    btnInit();
    powerIoInit();
    batEN(1);
    // powerManagerAtStart();

    // --- rgb led & bootRgbAnimationStop ---
    rgbLEDInit();
    bootRGBAnimationStart();

    // --- fill info ---
    dataBuffer[140] = BOARD_ID; // board_id
    dataBuffer[128] = VERSION_MAJOR;
    dataBuffer[129] = VERSION_MINOR;
    dataBuffer[130] = VERSION_MICRO;

    // --- peripherals init ---
    i2cInit();
    fanInit();
    chgCrntCtrlInit();
    tim0Init();

    // --- print info ---
    if (millis() - startTime < 1000)
    {
        delay(startTime + 1000 - millis());
    }
    // delay(1000);      // wait for PC to detect the serial port
    Serial.println(); // new line
    Serial.println(F("##################################################"));
    Serial.printf("SPC firmware %s\n", VERSION);
    Serial.println(); // new line

    // --- battery capacity init ---
    batCapacityInit();

    // --- whether ouput ---
    powerManagerAtStart();

    // --- SD init & read config file form SD ---
    SD_Init();
    sdDetectEventInit();
    if (hasSD)
    {
        createDir(SD, HISTORY_ROOT_DIR);
        listDir(SD, "/", 2);
        // --- read config file form SD ---
        readConfigFormSD(true);
    }

    // ---load config-- -
    loadPreferences();
    settingBuffer[0] = config.fanPower;
    settingBuffer[9] = config.shutdownPct;
    fanSet(config.fanPower);

    // --- wifi connecting ---
    wifiStart();
    delay(10);

    // --- mDNS ---
    Serial.println();
    mDNSInit(config.hostname);

    // --- webpage ---
    info("Webpage start ...");
    WebpageConfig webConfig;
    webConfig.version = VERSION;
    webConfig.onData = myOnData;
    webPageBegin(&webConfig);

    // --- MQTT ---
    // mqttClientStart(
    //     "192.168.137.1", 1883,
    //     "admin", "mqtt1234",
    //     mqttCallback);
    // mqttClient.subscribe("test");

    // --- time sync ---
    if (config.autoTimeEnable)
    {
        ntpTimeInit();
    }

    // --- bootRgbAnimationStop ---
    bootRgbAnimationStop();
    delay(100);
}

void loop()
{
    keyStateHandler();
    keyEventHandler();

    if (time50Cnt >= 50)
    {
        time50Cnt = 0;

        // if (test_flag)
        // {
        //     test_flag = 0;
        //     testPinSet(1);
        // }
        // else
        // {
        //     test_flag = 1;
        //     testPinSet(0);
        // }
        updateShutdownPct();
        updateFanPower();
        powerManage();
        rgbHandler();
    }

    // if (time1000Cnt > 1000)
    // {
    //     Serial.printf("free: %d\n", ESP.getFreeHeap());

    //     // mqttClient.publish("test", "esp32-spc");
    //     // saveDataIntoSD();
    //     time1000Cnt = 0;
    //     // Serial.println("loop ...");
    // }

    // TODO: if wifi disconnect, then reconnect, interval 3s
    delayMicroseconds(50);
}
