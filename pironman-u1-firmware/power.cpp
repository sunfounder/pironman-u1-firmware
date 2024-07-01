#include "power.h"

uint8_t outputState = 0; // 0, off; 1, on; 2, wait for shutdown
bool powerSource = false;

uint16_t chg3V3Volt = 0;
uint16_t batCrntRefVolt = 0;
uint16_t usbVolt = 0;
uint16_t batVolt = 0;
uint16_t outputVolt = 0;

int16_t batCrnt = 0;
uint16_t usbCrnt = 0;
uint16_t outputCrnt = 0;

bool isBatPlugged = false;
bool isBatPowered = false;
uint8_t batPct = 0;
float batCapacity = 0;

bool isUsbPlugged = false;
bool isCharging = false;
bool isLowBattery = false;

// --- average ---
uint8_t avgFilterIndex = 0;

uint16_t usbVoltFilterBuf[AVERAGE_FILTER_SIZE] = {0};
uint16_t usbCrntFilterBuf[AVERAGE_FILTER_SIZE] = {0};
uint16_t batVoltFilterBuf[AVERAGE_FILTER_SIZE] = {0};
int16_t batCrntFilterBuf[AVERAGE_FILTER_SIZE] = {0};
uint16_t outputVoltFilterBuf[AVERAGE_FILTER_SIZE] = {0};
uint16_t outputCrntFilterBuf[AVERAGE_FILTER_SIZE] = {0};

uint32_t usbVoltFilterSum = 0;
uint32_t usbCrntFilterSum = 0;
uint32_t batVoltFilterSum = 0;
int32_t batCrntFilterSum = 0;
uint32_t outputVoltFilterSum = 0;
uint32_t outputCrntFilterSum = 0;

uint16_t usbVoltAvg = 0;
uint16_t usbCrntAvg = 0;
uint16_t batVoltAvg = 0;
int16_t batCrntAvg = 0;
uint16_t outputVoltAvg = 0;
uint16_t outputCrntAvg = 0;

/* ----------------------------------------------------------------
INPUT:
    BTN                 -> IO21 (Separate initialization)
    DEFAULT_ON          -> IO39
    Power_Source_1V8    -> IO42

OUTPUT:
    BAT_EN -> IO11
    DC_EN -> IO15
    USB_EN -> IO16
    PI5_BTN -> IO41 (Separate initialization)
----------------------------------------------------------------*/
// void powerIoInit(void)
// {
//     // --- input ---
//     pinMode(DEFAULT_ON_PIN, INPUT);
//     pinMode(POWER_SOURCE_PIN, INPUT);

//     // --- output ---
//     pinMode(DC_EN_PIN, OUTPUT);
//     pinMode(USB_EN_PIN, OUTPUT);
//     pinMode(BAT_EN_PIN, OUTPUT);

//     batEN(1);

//     if (DEFAULT_ON)
//     {
//         powerOutOpen(); // output open
//     }
//     else if (BTN)
//     {
//         powerOutOpen();
//     }
//     else
//     {
//         delay(2); // software anti-shaking
//         if (DEFAULT_ON || BTN)
//         {
//             powerOutOpen(); // output open
//         }
//         else
//         {
//             powerOutClose(); // output close
//         }
//     }
// }

// void powerSourcePinInit()
// {
//     pinMode(POWER_SOURCE_PIN, INPUT);
// }

bool checkDefaultOn()
{
    return digitalRead(DEFAULT_ON_PIN);
    // return !digitalRead(DEFAULT_ON_PIN); // reverse
}

bool checkPowerSouce()
{
    powerSource = digitalRead(POWER_SOURCE_PIN);
    return powerSource;
}

void powerOutOpen()
{
    batEN(1);
    dcEN(1);
    // usbEN(1);
    outputState = 1;
}

void powerOutClose()
{
    dcEN(0);
    usbEN(0);
    outputState = 0;
}

void pi5BtnSingleClick()
{
    pinMode(PI5_BTN_PIN, OUTPUT);
    //
    digitalWrite(PI5_BTN_PIN, LOW);
    delay(50);
    digitalWrite(PI5_BTN_PIN, HIGH);
    //
    pinMode(PI5_BTN_PIN, INPUT);
}

void pi5BtnDoubleClick()
{
    pinMode(PI5_BTN_PIN, OUTPUT);
    //
    digitalWrite(PI5_BTN_PIN, LOW);
    delay(100);
    digitalWrite(PI5_BTN_PIN, HIGH);
    delay(150);
    digitalWrite(PI5_BTN_PIN, LOW);
    delay(100);
    digitalWrite(PI5_BTN_PIN, HIGH);
    //
    pinMode(PI5_BTN_PIN, INPUT);
}

void batEN(bool sw)
{
    pinMode(BAT_EN_PIN, OUTPUT);

    if (sw)
    {
        digitalWrite(BAT_EN_PIN, HIGH);
    }
    else
    {
        digitalWrite(BAT_EN_PIN, LOW);
    }
}

void dcEN(bool sw)
{
    pinMode(DC_EN_PIN, OUTPUT);

    if (sw)
    {
        digitalWrite(DC_EN_PIN, HIGH);
    }
    else
    {
        digitalWrite(DC_EN_PIN, LOW);
    }
}

void usbEN(bool sw)
{
    pinMode(USB_EN_PIN, OUTPUT);

    if (sw)
    {
        digitalWrite(USB_EN_PIN, HIGH);
    }
    else
    {
        digitalWrite(USB_EN_PIN, LOW);
    }
}

/* ----------- requestShutDown -----------*/
uint8_t shutdownRequest = 0;

void requestShutDown(uint8_t code)
{
    shutdownRequest = code;
    outputState = 2; // wait for shutdown
    pi5BtnDoubleClick();
}
// adc
// =================================================================
// void adcInit(void)
// {
//     // adcAttachPin(pin);
//     // analogSetWidth(bits);
//     // analogSetAttenuation(attenuation);
//     // analogSetPinAttenuation(pin, attenuation);
//     // analogSetClockDiv(clockDiv);
//     // analogSetVRefPin(pin);a
//     // analogRead(pin);
//     // analogReadMilliVolts(pin);
//     ;
// }

uint16_t readADCVolt(uint8_t pin)
{
    // uint16_t val = AnalogRead(pin);
    // return val * 3300 / 8191;

    return (uint16_t)analogReadMilliVolts(pin);
}

uint16_t readChg3V3Volt(void)
{
    uint16_t volt = 0;
    volt = readADCVolt(CHG_3V3_PIN);
#if ADC_RAW_VOLT
    chg3V3Volt = volt;
#else
    chg3V3Volt = volt * CHG_3V3_GAIN;
#endif

    return chg3V3Volt;
}

uint16_t readBatCrntRefVolt()
{
    uint16_t volt = 0;
    volt = readADCVolt(BAT_CURRENT_REF_PIN);

#if ADC_RAW_VOLT
    batCrntRefVolt = volt;
#else
    batCrntRefVolt = volt;
#endif

    return batCrntRefVolt;
}

uint16_t readUsbVolt()
{
    uint16_t volt = 0;
    volt = readADCVolt(VBUS_3V3_PIN);

#if ADC_RAW_VOLT
    usbVolt = volt;
#else
    usbVolt = volt * VBUS_3V3_GAIN;
#endif

    return usbVolt;
}

uint16_t readBatVolt()
{
    uint16_t volt = 0;
    volt = readADCVolt(BAT_LV_3V3_PIN);

#if ADC_RAW_VOLT
    batVolt = volt;
#else
    batVolt = volt * BAT_LV_3V3_GAIN;
#endif

    return batVolt;
}

uint16_t readOutputVolt()
{
    uint16_t volt = 0;
    volt = readADCVolt(VOUT_3V3_PIN);

#if ADC_RAW_VOLT
    outputVolt = volt;
#else
    outputVolt = volt * OUTPUT_CURRENT_GAIN;
#endif

    return outputVolt;
}

int16_t readBatCrnt()
{
    uint16_t volt = 0;
    uint16_t refVolt = 0;
    volt = readADCVolt(BAT_CURRENT_PIN);
    refVolt = readADCVolt(BAT_CURRENT_REF_PIN);

#if ADC_RAW_VOLT
    batCrnt = volt;
#else
    batCrnt = (int16_t)(refVolt - volt) * BAT_CURRENT_GAIN;
#endif

    return (int16_t)batCrnt;
}

uint16_t readUsbCrnt()
{
    uint16_t volt = 0;
    volt = readADCVolt(USB_CURRENT_PIN);

#if ADC_RAW_VOLT
    usbCrnt = volt;
#else
    usbCrnt = volt * USB_CURRENT_GAIN;
#endif

    return usbCrnt;
}

uint16_t readOutputCrnt()
{
    uint16_t volt = 0;
    volt = readADCVolt(OUTPUT_CURRENT_PIN);

#if ADC_RAW_VOLT
    outputCrnt = volt;
#else
    outputCrnt = volt * OUTPUT_CURRENT_GAIN;
#endif

    return outputCrnt;
}

void powerDataFilter()
{
    // usb voltage
    usbVoltFilterSum = usbVoltFilterSum - usbVoltFilterBuf[avgFilterIndex] + usbVolt;
    usbVoltFilterBuf[avgFilterIndex] = usbVolt;
    usbVoltAvg = usbVoltFilterSum / AVERAGE_FILTER_SIZE;
    // usb current
    usbCrntFilterSum = usbCrntFilterSum - usbCrntFilterBuf[avgFilterIndex] + usbCrnt;
    usbCrntFilterBuf[avgFilterIndex] = usbCrnt;
    usbCrntAvg = usbCrntFilterSum / AVERAGE_FILTER_SIZE;
    // battery voltage
    batVoltFilterSum = batVoltFilterSum - batVoltFilterBuf[avgFilterIndex] + batVolt;
    batVoltFilterBuf[avgFilterIndex] = batVolt;
    batVoltAvg = batVoltFilterSum / AVERAGE_FILTER_SIZE;
    // battery current
    batCrntFilterSum = batCrntFilterSum - batCrntFilterBuf[avgFilterIndex] + batCrnt;
    batCrntFilterBuf[avgFilterIndex] = batCrnt;
    batCrntAvg = batCrntFilterSum / AVERAGE_FILTER_SIZE;
    // output voltage
    outputVoltFilterSum = outputVoltFilterSum - outputVoltFilterBuf[avgFilterIndex] + outputVolt;
    outputVoltFilterBuf[avgFilterIndex] = outputVolt;
    outputVoltAvg = outputVoltFilterSum / AVERAGE_FILTER_SIZE;
    // output current
    outputCrntFilterSum = outputCrntFilterSum - outputCrntFilterBuf[avgFilterIndex] + outputCrnt;
    outputCrntFilterBuf[avgFilterIndex] = outputCrnt;
    outputCrntAvg = outputCrntFilterSum / AVERAGE_FILTER_SIZE;
    // ----
    avgFilterIndex++;
    if (avgFilterIndex >= AVERAGE_FILTER_SIZE)
    {
        avgFilterIndex = 0;
    }
}

// chargeAdjust
// =================================================================
int dac_vol = 1500;

void chgCrntCtrlInit(void)
{
    pinMode(CHG_CRNT_CTRL_PIN, OUTPUT);
}

void chgCrntCtrl(uint16_t volt)
{
    uint16_t ccr = 0;
    ccr = (uint16_t)((uint32_t)volt * 255 / 3300);
    dacWrite(CHG_CRNT_CTRL_PIN, ccr);
    dac_vol = volt;
}

#define IP_MIN_TARGET_VOLTAGE 4900

#define KP 0.01
#define KI 0.0
#define KD 0.05

// #define KP 0.005
// #define KI 0.0
// #define KD 0.01

int lastError = 0;
long sumError = 0;

/** pid
 */
float pidCalculate(uint16_t current_vol)
{
    float error, dError, output;

    error = (int)current_vol - IP_MIN_TARGET_VOLTAGE; // error

    // sumError += error;                   // integration
    // if (sumError < -330000)
    //     sumError = -330000; // integration limit
    // else if (sumError > 330000)
    //     sumError = 330000;

    dError = error - lastError; // differential
    lastError = error;

    // output = KP * error + KI * sumError + KD * dError;
    output = KP * error + KD * dError;
    return output;
}

void chargeAdjust(bool _powerSource, uint16_t _vbusVolt)
{
    int pidOutput = 0;

    if (outputState == 0)
    {
        chgCrntCtrl(0);
        return;
    }

    if (_powerSource)
    {
        // stop charging
        chgCrntCtrl(1500);
    }
    else
    {
        pidOutput = (int)pidCalculate(_vbusVolt);
        dac_vol -= pidOutput;
        if (dac_vol > 1500)
            dac_vol = 1500;
        else if (dac_vol < 0)
            dac_vol = 0;
        chgCrntCtrl((uint16_t)dac_vol);
    }
}

// Battery capacitys
// =================================================================
#define VALID_BATTERY_IR_BURRENT_ERROR 500 // mA, When the difference in current exceeds 500mA, calculate the internal resistance.
#define CONFIG_PREFS_NAMESPACE "batteryIR"

float batIR = DEFAULT_BATTERY_IR;
uint16_t batChgVolt = 0;
int16_t batChgCrnt = 0;
uint16_t batDischgVolt = 0;
int16_t batDischgCrnt = 0;

void batIRInit()
{
    Preferences batIRPerf;
    batIRPerf.begin(CONFIG_PREFS_NAMESPACE);
    batIR = batIRPerf.getFloat(CONFIG_PREFS_NAMESPACE, DEFAULT_BATTERY_IR);
    batIRPerf.end();

    batChgVolt = readBatVolt();
    batChgCrnt = readBatCrnt();
    batDischgVolt = batChgVolt;
    batDischgCrnt = batChgCrnt;
}

void updateBatIR()
{
    int32_t batIRCrntErr = 0;

    // --- update val ---
    if (batCrnt > 200)
    {
        batChgVolt = batVolt;
        batChgCrnt = batCrnt;
    }
    else if (batCrnt < -200)
    {
        batDischgVolt = batVolt;
        batDischgCrnt = batCrnt;
    }

    // --- update battery IR ---
    batIRCrntErr = batChgCrnt - batDischgCrnt;

    if (batIRCrntErr > VALID_BATTERY_IR_BURRENT_ERROR)
    {
        batIR = (float)(batChgVolt - batDischgVolt) / batIRCrntErr;
    }

    // Serial.printf("batIR: %f\n", batIR);
}

void saveBatIR()
{
    Preferences batIRPerf;
    batIRPerf.begin(CONFIG_PREFS_NAMESPACE);
    batIRPerf.putFloat(CONFIG_PREFS_NAMESPACE, batIR);
    batIRPerf.end();
}

uint8_t batVolt2Pct(uint16_t volt)
{
    uint8_t percent = 0;

    if (volt > BAT_CAPACITY_MAX_VOLTAGE)
    {
        percent = 100;
    }
    else if (volt < BAT_CAPACITY_MIN_VOLTAGE)
    {
        percent = 0;
    }
    else
    {
        percent = (uint8_t)round2Int((volt - BAT_CAPACITY_MIN_VOLTAGE) * 100.0 / (BAT_CAPACITY_MAX_VOLTAGE - BAT_CAPACITY_MIN_VOLTAGE));
    }

    return percent;
}

uint16_t batVolt2Capacity(uint16_t volt)
{
    uint16_t capacity = 0;

    if (volt > BAT_CAPACITY_MAX_VOLTAGE)
    {
        capacity = MAX_CAPACITY;
    }
    else if (volt < BAT_CAPACITY_MIN_VOLTAGE)
    {
        capacity = 0;
    }
    else if (volt > P7Voltage)
    {
        // capacity = MAX_CAPACITY*0.7 + (float)(volt - P7Voltage) / (MaxVoltage - P7Voltage) * MAX_CAPACITY * (1 - 0.07);
        capacity = 140 + (volt - P7Voltage) / 1440.0 * 1860;
    }
    else if (volt < P7Voltage)
    {
        // capacity = (float)(volt - MinVoltage) / (P7Voltage - MinVoltage) * MAX_CAPACITY * (0.07);
        capacity = (float)(volt - BAT_CAPACITY_MIN_VOLTAGE) / 300 * 140;
    }
    return capacity;
}

void batCapacityInit()
{
    uint32_t _sum = 0;

    // info("battery capacity init ...");

    for (int i = 0; i < 10; i++)
    {
        _sum += readBatVolt() - readBatCrnt() * DEFAULT_BATTERY_IR;
        delay(10);
    }
    batCapacity = (float)(batVolt2Capacity((uint16_t)(_sum / 10)));
}

// ----- updateBatCapacity -----
#define MAX_BAT_CALI_COUNT 40         // 40*50ms = 2s
#define BAT_CAPACITY_CALC_INTERVAL 50 // ms
#define BAT_REINIT_CAPACITY_ERR 200   // mAh
#define MAX_BAT_REINITCOUNT 20        // 20*50ms = 1s

bool p7caliFlag = true;
uint8_t batCaliCount = 0;
uint8_t batCaliFactor = 0;

uint8_t batReInitCount = 0;

void updateBatCapacity()
{
    uint16_t realVolt = 0;
    uint16_t _volt2Capa = 0;
    int32_t _capaErr = 0;

    // --- battery unplugged ---
    if (!isBatPlugged)
    {
        // Serial.println("BatPlugged out");
        batCapacity = 0.0;
        return;
    }

    // --- realVolt ---
    // batCrnt : + charge, - discharge
    realVolt = batVolt - batCrnt * DEFAULT_BATTERY_IR;

    // --- check if the voltage is stable ---
    _volt2Capa = batVolt2Capacity(realVolt);

    _capaErr = batCapacity - _volt2Capa;

    // Serial.printf("111: _volt2Capa: %d\n", _volt2Capa);
    // Serial.printf("2222: _capaErr: %d\n", _capaErr);

    if (_capaErr > BAT_REINIT_CAPACITY_ERR || _capaErr < -BAT_REINIT_CAPACITY_ERR)
    {
        batReInitCount += 1;
        if (batReInitCount > MAX_BAT_REINITCOUNT)
        {
            // Battery Capacity reinit
            // Serial.println("Battery Capacity reinit");
            batReInitCount = 0;
            batCapacityInit();
            return;
        }
    }
    else
    {
        batReInitCount = 0;
    }

    // --- voltage calibration point ---
    if (realVolt < P7Voltage)
    {
        if (p7caliFlag)
        {
            batCaliCount++;
            if (batCaliCount > MAX_BAT_CALI_COUNT)
            {
                batCapacity = (float)MAX_CAPACITY * 0.07;
                p7caliFlag = false;
                return;
            }
        }
    }
    else
    {
        batCaliCount = 0;
        if (realVolt > P7Voltage + 500)
        {
            p7caliFlag = true;
        }
    }

    // --- current integral ---
    batCapacity += batCrnt * (float)BAT_CAPACITY_CALC_INTERVAL / 3600.0 / 1000.0;
    if (batCapacity >= MAX_CAPACITY)
    {
        batCapacity = (float)MAX_CAPACITY;
    }
    else if (batCapacity <= 0)
    {
        batCapacity = 0.0;
    }

    // Serial.printf("xxx: capacity: %f\n", batCapacity);
}

uint8_t batCapacity2Pct(uint16_t capacity)
{
    uint8_t pct = 0;
    pct = (uint8_t)round2Int((capacity / (float)MAX_CAPACITY * 100));
    return pct;
}

int round2Int(float num)
{
    int rounded = (int)(num + 0.05);
    return rounded;
}
