#include "sdcard.h"

bool hasSD = 0;
SPIClass *sdSpi = NULL;

bool SD_Init(void)
{
    pinMode(SD_DT, INPUT_PULLUP);

    sdSpi = new SPIClass(SD_SPI);
    sdSpi->begin(SD_SCLK, SD_MISO, SD_MOSI, SD_SS);
    pinMode(sdSpi->pinSS(), OUTPUT);

    // SD.begin(uint8_t ssPin=SS, SPIClass &spi=SPI, uint32_t frequency=4000000, const char * mountpoint="/sd", uint8_t max_files=5, bool format_if_empty=false);
    if (!SD.begin(SD_SS, *sdSpi, SD_SPI_FREQ, SD_MOUNT_POINT, SD_MAX_FILES, false))
    {
        Serial.println("SD card Mount Failed\n");
        hasSD = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE)
    {
        Serial.println("No SD card attached");
        return false;
    }

    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC)
    {
        Serial.println("MMC");
    }
    else if (cardType == CARD_SD)
    {
        Serial.println("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
        Serial.println("SDHC");
    }
    else
    {
        Serial.println("UNKNOWN");
    }

    Serial.printf("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    Serial.println();

    hasSD = true;
    return true;
}

bool checkSD_DT()
{
    return !digitalRead(SD_DT); // reverse
}

#if 0
void sdDetectEvent()
{
    debug("sdDetectEvent: ");
    if (checkSD_DT())
    {
        debug("SD_DT = true\n");
        // delay(50);
        // Serial.printf("free: %d\n", ESP.getFreeHeap());
        // SD_Init();
    }
    else
    {
        debug("SD_DT = false\n");
        hasSD = false;
        // SD.end();
    }
}

void sdDetectEventInit()
{
    pinMode(SD_DT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(SD_DT), sdDetectEvent, CHANGE);
}
#endif
