#pragma once

#include <SD.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "debug.h"
#include "file_system.h"

/*----------------------------------------------------------------*/
#define CONFIG_PREFS_NAMESPACE "conifg"

#define SD_CONFIG_FILE "/config.json"

#define HOSTNAME_KEYNAME "hostname"
#define STA_ENABLE_KEYNAME "staENABLE"
#define STA_SSID_KEYNAME "staSSID"
#define STA_PSK_KEYNAME "staPSK"
#define AP_SSID_KEYNAME "apSSID"
#define AP_PSK_KEYNAME "apPSK"
#define AUTO_TIME_ENABLE_KEYNAME "autoTimeEnable"
#define TIME_ZONE_KEYNAME "timezone"
#define NTP_SERVER_KEYNAME "ntpServe"
#define SHUTDOWN_PCT_KEYNAME "shutdownPct"
#define FAN_POWER_KEYNAME "fanPower"
#define SD_DATA_INTERVAL_KEYNAME "sdDataInterval"
#define SD_DATA_RETAIN_KEYNAME "sdDataRetain"

#define DEFAULT_HOSTNAME "Pironman-U1"
#define DEFAULT_STA_ENABLE 0
#define DEFAULT_STA_SSID ""
#define DEFAULT_STA_PSK ""
#define DEFAULT_AP_SSID "Pironman-U1"
#define DEFAULT_AP_PSK "12345678"

#define DEFAULT_AUTO_TIME_ENABLE 1
#define DEFAULT_TIME_ZONE "UTC-08:00" // POSIX time zone
#define DEFAULT_NTP_SERVER "pool.ntp.org"

#define DEFAULT_SHUTDOWN_PCT 20

#define DEFAULT_FAN_POWER 75

#define DEFAULT_SD_DATA_INTERVAL 1 // seconds
#define DEFAULT_SD_DATA_RETAIN 7   // days

struct Config
{
    char hostname[32];
    bool staENABLE;
    char staSSID[32];
    char staPSK[32];
    char apSSID[32];
    char apPSK[32];
    bool autoTimeEnable;
    char timezone[12];
    char ntpServe[32];
    uint8_t shutdownPct;
    uint8_t fanPower;
    uint32_t sdDataInterval;
    uint32_t sdDataRetain;
};

extern Config config;
extern Preferences configPrefs;

void loadPreferences();
bool readConfigFormSD(bool isDelete);
