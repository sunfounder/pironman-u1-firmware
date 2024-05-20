#pragma once

#include <WiFi.h>

/*
<esp_wifi_types.h>

typedef enum {
    WIFI_MODE_NULL = 0,  //< null mode
    WIFI_MODE_STA,       //< WiFi station mode
    WIFI_MODE_AP,        //< WiFi soft-AP mode
    WIFI_MODE_APSTA,     //< WiFi station + soft-AP mode
    WIFI_MODE_MAX
} wifi_mode_t;

typedef enum {
    WIFI_AUTH_OPEN = 0,         // < authenticate mode : open
    WIFI_AUTH_WEP,              // < authenticate mode : WEP
    WIFI_AUTH_WPA_PSK,          // < authenticate mode : WPA_PSK
    WIFI_AUTH_WPA2_PSK,         // < authenticate mode : WPA2_PSK
    WIFI_AUTH_WPA_WPA2_PSK,     // < authenticate mode : WPA_WPA2_PSK
    WIFI_AUTH_WPA2_ENTERPRISE,  // < authenticate mode : WPA2_ENTERPRISE
    WIFI_AUTH_WPA3_PSK,         // < authenticate mode : WPA3_PSK
    WIFI_AUTH_WPA2_WPA3_PSK,    // < authenticate mode : WPA2_WPA3_PSK
    WIFI_AUTH_WAPI_PSK,         // < authenticate mode : WAPI_PSK
    WIFI_AUTH_MAX
} wifi_auth_mode_t;
*/
#define NONE WIFI_MODE_NULL
#define STA WIFI_MODE_STA
#define AP WIFI_MODE_AP
#define APSTA WIFI_MODE_MAX

class WiFiHelper
{
public:
    WiFiHelper();
    String apIP = "";
    String staIP = "";
    bool is_connected = false;
    void setMode(wifi_mode_t mode);
    void closeSTA();
    bool connectSTA(String ssid, String password);
    bool connectAP(String ssid, String password);
    String getAPIP();
    String getSTAIP();
    bool checkStatus();
};
