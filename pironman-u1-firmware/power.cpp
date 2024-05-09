#include "power.h"

uint8_t outputState = 0; // 0, off; 1, on; 2, wait for shutdown

uint16_t chg3V3Volt = 0;
uint16_t batCrntRefVolt = 0;
uint16_t usbVolt = 0;
uint16_t batVolt = 0;
uint16_t outputVolt = 0;

int16_t batCrnt = 0;
uint16_t usbCrnt = 0;
uint16_t outputCrnt = 0;

uint8_t isBatPlugged = 0;
uint8_t batPct = 0;

uint8_t isUsbPlugged = 0;
uint8_t isCharging = 0;
uint8_t isLowBattery = 0;
uint16_t powerSource = 0;

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
    ALWAYS_ON -> IO39
    BTN -> IO21

OUTPUT:
    BAT_EN -> IO11
    DC_EN -> IO15
    USB_EN -> IO16
    PI5_BTN -> IO41
----------------------------------------------------------------*/
void powerIoInit(void)
{
    pinMode(ALWAYS_ON_PIN, INPUT);
    pinMode(POWER_SOURCE_PIN, INPUT);

    pinMode(DC_EN_PIN, OUTPUT);
    pinMode(USB_EN_PIN, OUTPUT);
    pinMode(BAT_EN_PIN, OUTPUT);

    digitalWrite(DC_EN_PIN, LOW);
    digitalWrite(USB_EN_PIN, LOW);
    pinMode(BAT_EN_PIN, HIGH);
}

uint8_t isAlwaysOn()
{
    // return digitalRead(ALWAYS_ON_PIN);
    return !digitalRead(ALWAYS_ON_PIN); // reverse
}

uint8_t checkPowerSouce()
{
    powerSource = digitalRead(POWER_SOURCE_PIN);
    return powerSource;
}

void powerOutOpen()
{
    digitalWrite(DC_EN_PIN, HIGH);
    digitalWrite(USB_EN_PIN, HIGH);
    outputState = 1;
}

void powerOutClose()
{
    digitalWrite(DC_EN_PIN, LOW);
    digitalWrite(USB_EN_PIN, LOW);
    outputState = 0;
}

void pi5BtnDoubleClick()
{
    pinMode(PI5_BTN_PIN, OUTPUT);
    //
    digitalWrite(PI5_BTN_PIN, LOW);
    delay(50);
    digitalWrite(PI5_BTN_PIN, HIGH);
    delay(50);
    digitalWrite(PI5_BTN_PIN, LOW);
    delay(50);
    digitalWrite(PI5_BTN_PIN, HIGH);
    //
    pinMode(PI5_BTN_PIN, INPUT);
}

/* ----------- requestShutDown -----------*/
uint8_t shutdownRequest = 0;

void requestShutDown(uint8_t code)
{
    shutdownRequest = code;
    outputState = 2; // wait for shutdown
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
    // Serial.printf("bat_vlot: %d\n", volt);

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
// Battery capacitys
// =================================================================
uint8_t calcBatPct()
{
    uint8_t percent = 0;

    if (batVolt > MAX_BAT_VOLTAGE)
    {
        percent = 100;
    }
    else if (batVolt < MIN_BAT_VOLTAGE)
    {
        percent = 0;
    }
    else
    {
        percent = (batVolt - MIN_BAT_VOLTAGE) * 100 / (MAX_BAT_VOLTAGE - MIN_BAT_VOLTAGE);
    }

    batPct = percent;
    return batPct;
}

// chargeAdjust
// =================================================================
void chgCrntCtrlInit(void)
{
    pinMode(CHG_CRNT_CTRL_PIN, OUTPUT);
}

void chgCrntCtrl(uint16_t volt)
{
    uint16_t ccr = 0;
    ccr = (uint16_t)((uint32_t)volt * 255 / 3300);
    dacWrite(CHG_CRNT_CTRL_PIN, ccr);
}

#define IP_MIN_TARGET_VOLTAGE 4900

#define KP 0.01
#define KI 0.0
#define KD 0.05

int dac_vol = 1500;
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

void chargeAdjust(uint16_t _powerSource, uint16_t _vbusVolt)
{
    int pidOutput = 0;

    if (_powerSource)
    {
        // stop charging
        chgCrntCtrl(1500);
        dac_vol = 1500;
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