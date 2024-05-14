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

USB:
    D+ -> USB_D+
    D- -> USB_D-

BOARD_ID:
    BD0 -> IO40
    BD1 -> IO41
    BD2 -> IO42

PWM:
    RGB_LED:
        LED_R -> IO26
        LED_G -> IO33
        LED_B -> IO38

    FAN:
        FAN -> IO18

DAC:
    CHG_CRNT_CTRL -> IO17

INPUT:
    ALWAYS_ON -> IO39
    BTN -> IO21

OUTPUT:
    DC_EN -> IO15
    USB_EN -> IO16

ADC:
    CHG_3V3 -> IO1
    BATTERY_CURRENT_REF -> IO2
    BATTERY_CURRENT_FILTERED -> IO3
    USB_CURRENT_FILTERED -> IO4
    Power_Source_3V3 -> IO5
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
#include "deep_sleep.h"
#include "file_system.h"
#include "LittleFS.h"
#include "mqtt.h"
#include "date_time.h"
#include "config.h"

#define SPIFFS LittleFS

// VERSION INFO
// =================================================================
#define VERSION "0.0.6"
#define VERSION_MAJOR 0
#define VERSION_MINOR 0
#define VERSION_MICRO 6

#define BOARD_ID 0x00

// global variables
// =================================================================
bool hasSD = 0;
WiFiHelper wifi = WiFiHelper();

// Functions
// =================================================================
void saveDataIntoSD();
void shutdown();
// void rpiStateHandler();

// -----------------

void wifiStart()
{
    Serial.println("\nWiFi start ...");
    // aps
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

bool myOnConfig(int mode, bool enable, String ssid, String psk)
{
    String txtbuf;
    size_t size;

    // loading config in SPIFFS
    readFile(SPIFFS, SPIFFS_CONFIG_FILE, &txtbuf, &size);

    DynamicJsonDocument doc(size + 64);
    DeserializationError _error = deserializeJson(doc, txtbuf);
    if (_error)
        error("Failed to read file, using default configuration");

    // update config
    if (mode == STA)
    {
        doc["STA"]["ENABLE"] = enable;
        doc["STA"]["SSID"] = ssid;
        doc["STA"]["PSK"] = psk;
    }
    else if (mode == AP)
    {
        doc["AP"]["SSID"] = ssid;
        doc["AP"]["PSK"] = psk;
    }

    // write config
    serializeJson(doc, txtbuf);
    serializeJsonPretty(doc, txtbuf);
    writeFile(SPIFFS, SPIFFS_CONFIG_FILE, txtbuf.c_str());

    // clear
    doc.clear();

    // return execution status
    return true;
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

    Serial.print("onReceive: ");
    Serial.println(numBytes);

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
// time counter (TIM1) 5ms interrupt
// =================================================================
#define TIM0_COUNTER_PREIOD 5 // ms
#define TIM0_DIV 80           // 80MHz / 80 = 1MHz, count plus one every us
#define TIM0_TICKS 5000       // 5000 * us = 5ms

hw_timer_t *tim0 = NULL;
uint16_t time20Cnt = 0;
uint16_t time1000Cnt = 0;

void IRAM_ATTR onTim0()
{
    keyScanTimeCnt += TIM0_COUNTER_PREIOD;
    longPressTimeCnt += TIM0_COUNTER_PREIOD;
    doulePressTimeCnt += TIM0_COUNTER_PREIOD;
    rgbTimeCnt += TIM0_COUNTER_PREIOD;
    time20Cnt += TIM0_COUNTER_PREIOD;
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
// power manager
// =================================================================
#define LOW_BATTERY_WARNING 20          // 20% Low battery warning（led flashing）
#define LOW_BATTERY_SHUTDOWM_REQUEST 10 // 10% Low battery shutdown request
#define LOW_BATTERY_POWER_OFF 5         // 5% Force shutdown
#define LOW_BATTERY_MAX_COUNT 250       // 5000 / 20ms, Confirm low battery for 5 seconds continuously

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
    calcBatPct();
    checkPowerSouce();

    // ---- status judgment ----
    // isUsbPlugged
    if (usbVolt > USB_MIN_VOLTAGE)
    {
        isUsbPlugged = 1;
    }
    else
    {
        isUsbPlugged = 0;
    }

    // isCharging
    if (batCrnt > 100)
    {
        isCharging = 1;
    }
    else if (batCrnt < 20)
    {
        isCharging = 0;
    }

    // isLowPower
    if (batPct < LOW_BATTERY_WARNING)
    {
        isLowBattery = 1;
    }
    else
    {
        isLowBattery = 0;
    }

    // ---- set rgb mode ----
    if (isCharging)
    {
        rgbMode = RGB_MODE_CHARGING;
    }
    else if (isLowBattery)
    {
        rgbMode = RGB_MODE_LOW_BATTERY;
    }
    else if (powerSource == 1)
    {
        rgbMode = RGB_MODE_BAT_POWERED;
    }
    else if (powerSource == 0)
    {
        rgbMode = RGB_MODE_USB_POWERED;
    }

    // ---- fill data into dataBuffer ----
    dataBuffer[1] = chg3V3Volt >> 8 & 0xFF;
    dataBuffer[2] = chg3V3Volt & 0xFF;

    dataBuffer[3] = usbVolt >> 8 & 0xFF;
    dataBuffer[4] = usbVolt & 0xFF;

    dataBuffer[5] = usbCrnt >> 8 & 0xFF;
    dataBuffer[6] = usbCrnt & 0xFF;

    dataBuffer[7] = outputVolt >> 8 & 0xFF;
    dataBuffer[8] = outputVolt & 0xFF;

    dataBuffer[9] = outputCrnt >> 8 & 0xFF;
    dataBuffer[10] = outputCrnt & 0xFF;

    dataBuffer[11] = batVolt >> 8 & 0xFF;
    dataBuffer[12] = batVolt & 0xFF;

    dataBuffer[13] = ((uint16_t)batCrnt) >> 8 & 0xFF;
    dataBuffer[14] = ((uint16_t)batCrnt) & 0xFF;

    dataBuffer[22] = shutdownRequest;

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

    dataDoc["data"]["output_voltage"] = outputVolt;
    dataDoc["data"]["output_current"] = outputCrnt;

    dataDoc["data"]["output_state"] = outputState;
    dataDoc["data"]["power_source"] = powerSource;

    powerDataFilter();
}

/** lowBatteryHandler
 */
#define LOW_BATTERY_WARNING 20          // 20% Low battery warning（led flashing）
#define LOW_BATTERY_SHUTDOWM_REQUEST 10 // 10% Low battery shutdown request
#define LOW_BATTERY_POWER_OFF 5         // 5% Force shutdown
#define LOW_BATTERY_MAX_COUNT 250       // 5000 / 20ms, Confirm low battery for 5 seconds continuously

uint16_t lowPowerCount = 0;
uint8_t shutdown_pct = 0;
uint8_t poweroff_pct = 0;

void shutdownBatteryPctHandler()
{
    if (shutdown_pct != settingBuffer[9])
    {
        shutdown_pct = settingBuffer[9];
        dataBuffer[143] = shutdown_pct;

        prefs.begin(PREFS_NAMESPACE); // namespace
        prefs.putUChar(SHUTDOWN_PCT_KEYNAME, shutdown_pct);
        prefs.end();

        config.shutdownPct = shutdown_pct;
    }

    if (poweroff_pct != settingBuffer[10])
    {
        poweroff_pct = settingBuffer[10];
        dataBuffer[144] = poweroff_pct;

        prefs.begin(PREFS_NAMESPACE); // namespace
        prefs.putUChar(POWEROFF_PCT_KEYNAME, poweroff_pct);
        prefs.end();

        config.poweroffPct = poweroff_pct;
    }
}

/* ----------- shutdown -----------*/
void shutdown()
{
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
        // powerSave();
    }
}

/* ----------- lowBatteryHandler -----------*/
void lowBatteryHandler()
{
    // usb 插入时不处理
    if (isUsbPlugged)
    {
        lowPowerCount = 0;
    }
    else
    {
        if (batPct < LOW_BATTERY_SHUTDOWM_REQUEST)
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
                    shutdown(); // 5% Force shutdown
                }
            }
        }
        else
        {
            lowPowerCount = 0;
        }
    }
}

/* ----------- powerManage -----------*/
void powerManage()
{
    updatePowerData();
    // chargeAdjust(powerSourceVolt, usbVolt);
    shutdownBatteryPctHandler();

    // lowBatteryHandler();
    // rpiStateHandler();
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
        keyState = Released;
        powerOutOpen();
        rgbWrite(0, 255, 0);
        delay(100);
        rgbClose();
        break;
    case DouleClicked:
        keyState = Released;
        rgbWrite(0, 0, 255);
        delay(100);
        rgbClose();
        pi5BtnDoubleClick();
        enterDeepSleep();
        break;
    case LongPressed2S:
        rgbWrite(255, 120, 0);
        break;
    case LongPressed2SAndReleased:
        keyState = Released;
        rgbClose();
        //
        requestShutDown(2); // 2，button shutdown request
        rgbMode = RGB_MODE_WAIT_SHUTDOWN;
        break;
    case LongPressed5S:
        rgbWrite(255, 0, 0);
        break;
    case LongPressed5SAndReleased:
        keyState = Released;
        rgbClose();
        //
        // shutdown();
        break;
    default:
        break;
    }
}

// setup & loop
// =================================================================
void setup()
{
    Serial.begin(115200);

    // --- fill info ---
    dataBuffer[140] = BOARD_ID; // board_id
    dataBuffer[128] = VERSION_MAJOR;
    dataBuffer[129] = VERSION_MINOR;
    dataBuffer[130] = VERSION_MICRO;

    shutdown_pct = LOW_BATTERY_SHUTDOWM_REQUEST;
    poweroff_pct = LOW_BATTERY_POWER_OFF;
    dataBuffer[143] = shutdown_pct;
    settingBuffer[9] = shutdown_pct;
    dataBuffer[144] = poweroff_pct;
    settingBuffer[10] = poweroff_pct;

    // --- peripherals init ---
    rgbLEDInit();
    rgbWrite(255, 255, 255); // Initializing indicator light

    powerIoInit();
    fanInit();
    btnInit();
    tim0Init();
    i2cInit();
    chgCrntCtrlInit();

    // --- print info ---
    delay(1000);      // wait for PC to detect the serial port
    Serial.println(); // new line
    Serial.println("##################################################");
    Serial.printf("SPC firmware %s\n", VERSION);
    Serial.println(); // new line

    ++bootCount;
    // Serial.println("Boot number: " + String(bootCount));
    info("Boot number: %d", bootCount);
    print_wakeup_reason();
    print_GPIO_wake_up();
    Serial.println(); // new line

    // --- power output at start ---
    // powerManagerAtStart()

    // --- SPIFFS (LittleFS) init ---
    if (!SPIFFS.begin())
    {
        listDir(SPIFFS, "/", 2);
    }

    // --- SD init & read config file form SD ---
    hasSD = SD_Init();
    if (hasSD)
    {
        createDir(SD, HISTORY_ROOT_DIR);
        listDir(SD, "/", 2);
        // --- read config file form SD to SPIFFS---
        // copyConfigFromSD2SPIFFS();
        readConfigFormSD(true);
    }
    // SD.end();

    // --- load config ---
    loadPreferences();

    // --- wifi connecting ---
    wifiStart();
    delay(10);

    // -- -mDNS &webpage-- -
    Serial.println();
    mDNSInit(config.hostname);

    info("Webpage start ...");
    WebpageConfig webConfig;
    webConfig.version = VERSION;
    webConfig.onData = myOnData;
    webConfig.onConfig = myOnConfig;
    webPageBegin(&webConfig);

    // --- MQTT ---
    // mqttClientStart(
    //     "192.168.137.1", 1883,
    //     "admin", "mqtt1234",
    //     mqttCallback);
    // mqttClient.subscribe("test");

    // --- time sync ---
    ntpTimeInit();
}

uint8_t fanSpeed = 0;
void loop()
{

    keyStateHandler();
    keyEventHandler();

    if (time20Cnt >= 20)
    {
        time20Cnt = 0;

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

        chgCrntCtrl(0);

        webPageLoop();
        powerManage();
        rgbHandler();

        fanSpeed = 100;
        dataDoc["data"]["fan_power"] = fanSpeed;
        fanSet(100);
    }

    if (time1000Cnt > 1000)
    {
        Serial.printf("free: %d\n", ESP.getFreeHeap());

        // mqttClient.publish("test", "esp32-spc");
        saveDataIntoSD();
        time1000Cnt = 0;
        // Serial.println("loop ...");
    }

    // TODO: if wifi disconnect, then reconnect, interval 3s
    delayMicroseconds(50);
}
