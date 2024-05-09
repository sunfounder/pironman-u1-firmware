#include "config.h"

Preferences prefs;
Config config;

void loadPreferences()
{
    Serial.println(); // new line
    info("Loading preferences ...");

    prefs.begin(PREFS_NAMESPACE); // namespace

    config.staENABLE = prefs.getUChar(STA_ENABLE_KEYNAME, DEFAULT_STA_ENABLE);
    strlcpy(config.hostname, prefs.getString(HOSTNAME_KEYNAME, DEFAULT_HOSTNAME).c_str(), sizeof(config.hostname));
    strlcpy(config.staSSID, prefs.getString(STA_SSID_KEYNAME, DEFAULT_STA_SSID).c_str(), sizeof(config.staSSID));
    strlcpy(config.staPSK, prefs.getString(STA_PSK_KEYNAME, DEFAULT_STA_PSK).c_str(), sizeof(config.staPSK));
    strlcpy(config.apSSID, prefs.getString(AP_SSID_KEYNAME, DEFAULT_AP_SSID).c_str(), sizeof(config.apSSID));
    strlcpy(config.apPSK, prefs.getString(AP_PSK_KEYNAME, DEFAULT_AP_PSK).c_str(), sizeof(config.apPSK));
    strlcpy(config.timezone, prefs.getString(TIME_ZONE_KEYNAME, DEFAULT_TIME_ZONE).c_str(), sizeof(config.apPSK));
    strlcpy(config.ntpServe, prefs.getString(NTP_SERVER_KEYNAME, DEFAULT_NTP_SERVER).c_str(), sizeof(config.apPSK));

    prefs.end();

    info("hostname: %s", config.hostname);
    info("sta.enable: %d", config.staENABLE);
    info("sta.ssid: %s", config.staSSID);
    info("sta.psk: %s", config.staPSK);
    info("ap.ssid: %s", config.apSSID);
    info("ap.psk: %s", config.apPSK);
    info("timezone: %s", config.timezone);
    info("ntpServe: %s", config.ntpServe);
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
        prefs.begin(PREFS_NAMESPACE); // namespace

        if (doc.containsKey("hostname"))
        {
            prefs.putString(HOSTNAME_KEYNAME, doc["hostname"].as<const char *>());
        }
        if (doc.containsKey("sta"))
        {
            if (doc["sta"].containsKey("enable"))
            {
                prefs.putUChar(STA_ENABLE_KEYNAME, doc["sta"]["enable"].as<unsigned char>());
            }
            if (doc["sta"].containsKey("ssid"))
            {
                prefs.putString(STA_SSID_KEYNAME, doc["sta"]["ssid"].as<const char *>());
            }
            if (doc["sta"].containsKey("psk"))
            {
                prefs.putString(STA_PSK_KEYNAME, doc["sta"]["psk"].as<const char *>());
            }
        }
        if (doc.containsKey("ap"))
        {
            if (doc["ap"].containsKey("ssid"))
            {
                prefs.putString(AP_SSID_KEYNAME, doc["ap"]["ssid"].as<const char *>());
            }
            if (doc["ap"].containsKey("psk"))
            {
                prefs.putString(AP_PSK_KEYNAME, doc["ap"]["psk"].as<const char *>());
            }
        }
        if (doc.containsKey("timezone"))
        {
            prefs.putString(TIME_ZONE_KEYNAME, doc["timezone"].as<const char *>());
        }
        if (doc.containsKey("ntpServe"))
        {
            prefs.putString(TIME_ZONE_KEYNAME, doc["ntpServe"].as<const char *>());
        }

        // clear
        doc.clear();
        prefs.end();

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
