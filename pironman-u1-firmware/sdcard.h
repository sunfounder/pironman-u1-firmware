#pragma once

#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "ArduinoJson.h"
#include "debug.h"
/* ----------------------------------------------------------------
SD SPI:
    MOSI -> IO35
    MISO -> IO37
    SCLK -> IO36
    CS0 -> IO34

SD_DT -> IO40
---------------------------------------------------------------- */
#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define VSPI FSPI
#endif

#define SD_SPI VSPI
// #define SD_SPI HSPI

#define SD_MISO 37
#define SD_MOSI 35
#define SD_SCLK 36
#define SD_SS 34

#define SD_DT 40

#define SD_SPI_FREQ 4000000 // 4MHz

#define SD_MAX_FILES 5
#define SD_MOUNT_POINT "/sd"

extern bool hasSD;

bool SD_Init(void);
bool checkSD_DT();
void sdDetectEventInit();