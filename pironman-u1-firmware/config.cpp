#include "config.h"

Preferences configPrefs;
Config config;

void loadPreferences()
{
    Serial.println(); // new line
    info("Loading preferences ...");

    configPrefs.begin(CONFIG_PREFS_NAMESPACE); // namespace

    config.staENABLE = configPrefs.getUChar(STA_ENABLE_KEYNAME, DEFAULT_STA_ENABLE);
    strlcpy(config.hostname, configPrefs.getString(HOSTNAME_KEYNAME, DEFAULT_HOSTNAME).c_str(), sizeof(config.hostname));
    strlcpy(config.staSSID, configPrefs.getString(STA_SSID_KEYNAME, DEFAULT_STA_SSID).c_str(), sizeof(config.staSSID));
    strlcpy(config.staPSK, configPrefs.getString(STA_PSK_KEYNAME, DEFAULT_STA_PSK).c_str(), sizeof(config.staPSK));
    strlcpy(config.apSSID, configPrefs.getString(AP_SSID_KEYNAME, DEFAULT_AP_SSID).c_str(), sizeof(config.apSSID));
    strlcpy(config.apPSK, configPrefs.getString(AP_PSK_KEYNAME, DEFAULT_AP_PSK).c_str(), sizeof(config.apPSK));
    config.autoTimeEnable = configPrefs.getUChar(AUTO_TIME_ENABLE_KEYNAME, DEFAULT_AUTO_TIME_ENABLE);
    strlcpy(config.timezone, configPrefs.getString(TIME_ZONE_KEYNAME, DEFAULT_TIME_ZONE).c_str(), sizeof(config.apPSK));
    strlcpy(config.ntpServe, configPrefs.getString(NTP_SERVER_KEYNAME, DEFAULT_NTP_SERVER).c_str(), sizeof(config.apPSK));
    config.shutdownPct = configPrefs.getUChar(SHUTDOWN_PCT_KEYNAME, DEFAULT_SHUTDOWN_PCT);
    config.fanPower = configPrefs.getUChar(FAN_POWER_KEYNAME, DEFAULT_FAN_POWER);
    config.sdDataInterval = configPrefs.getUInt(SD_DATA_INTERVAL_KEYNAME, DEFAULT_SD_DATA_INTERVAL);
    config.sdDataRetain = configPrefs.getUInt(SD_DATA_RETAIN_KEYNAME, DEFAULT_SD_DATA_RETAIN);

    configPrefs.end();

    info("hostname: %s", config.hostname);
    info("sta.enable: %d", config.staENABLE);
    info("sta.ssid: %s", config.staSSID);
    info("sta.psk: %s", config.staPSK);
    info("ap.ssid: %s", config.apSSID);
    info("ap.psk: %s", config.apPSK);
    info("autoTimeEnable: %d", config.autoTimeEnable);
    info("ntpServe: %s", config.ntpServe);
    info("shutdownPct: %d", config.shutdownPct);
    info("fanPower: %d", config.fanPower);
    info("sdDataInterval: %d", config.shutdownPct);
    info("sdDataRetain: %d", config.sdDataRetain);
}

bool readConfigFormSD(bool isDelete)
{
    String txtbuf;
    size_t size;

    Serial.println(); // new line
    if (SD.exists(SD_CONFIG_FILE))
    {
        info("Find config file in SD");
        // read file into String form SD
        readFile(SD, SD_CONFIG_FILE, &txtbuf, &size);

        // Allocate a temporary JsonDocument
        DynamicJsonDocument doc(size);

        // Deserialize the JSON document
        DeserializationError _error = deserializeJson(doc, txtbuf);
        if (_error)
        {
            info("Failed to read config file from SD");
            return false;
        }

        Serial.println("\'\'\'");
        // serializeJson(doc, Serial);
        serializeJsonPretty(doc, Serial);
        Serial.println("\n\'\'\'");

        // --- copy the config ---
        configPrefs.begin(CONFIG_PREFS_NAMESPACE); // namespace

        if (doc.containsKey("hostname"))
        {
            configPrefs.putString(HOSTNAME_KEYNAME, doc["hostname"].as<const char *>());
        }
        if (doc.containsKey("sta"))
        {
            if (doc["sta"].containsKey("enable"))
            {
                configPrefs.putUChar(STA_ENABLE_KEYNAME, doc["sta"]["enable"].as<unsigned char>());
            }
            if (doc["sta"].containsKey("ssid"))
            {
                configPrefs.putString(STA_SSID_KEYNAME, doc["sta"]["ssid"].as<const char *>());
            }
            if (doc["sta"].containsKey("psk"))
            {
                configPrefs.putString(STA_PSK_KEYNAME, doc["sta"]["psk"].as<const char *>());
            }
        }
        if (doc.containsKey("ap"))
        {
            if (doc["ap"].containsKey("ssid"))
            {
                configPrefs.putString(AP_SSID_KEYNAME, doc["ap"]["ssid"].as<const char *>());
            }
            if (doc["ap"].containsKey("psk"))
            {
                configPrefs.putString(AP_PSK_KEYNAME, doc["ap"]["psk"].as<const char *>());
            }
        }
        if (doc.containsKey("timezone"))
        {
            configPrefs.putString(TIME_ZONE_KEYNAME, doc["timezone"].as<const char *>());
        }
        if (doc.containsKey("ntpServe"))
        {
            configPrefs.putString(TIME_ZONE_KEYNAME, doc["ntpServe"].as<const char *>());
        }

        // clear
        doc.clear();
        configPrefs.end();

        // --- delete the config ---
        if (isDelete)
        {
            info("delete congfig file in SD");
            deleteFile(SD, SD_CONFIG_FILE);
        }
    }
    else
    {
        info("Config file not found in SD");
        return false;
    }

    return true;
}
