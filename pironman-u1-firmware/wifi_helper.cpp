
#include "wifi_helper.h"

WiFiHelper::WiFiHelper()
{
}

void WiFiHelper::setMode(wifi_mode_t mode)
{
    WiFi.mode(mode);
}

void WiFiHelper::closeSTA()
{
    WiFi.enableSTA(false);
}

bool WiFiHelper::connectSTA(String ssid, String password)
{
    Serial.println(F("Connecting to WiFi ..."));
    Serial.print(F("sta_ssid:"));
    Serial.println(ssid);
    Serial.print(F("sta_psk:"));
    Serial.println(password);

    WiFi.enableSTA(true);
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait some time to connect to wifi
    int count = 0;
    Serial.print("[DEBUG] Connecting.");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
        count++;
        if (count > 30)
        {
            Serial.println("");
            Serial.println(WiFi.status());
            return false;
        }
    }
    Serial.println();
    staIP = WiFi.localIP().toString();
    Serial.println(staIP);
    return true;
}

bool WiFiHelper::connectAP(String ssid, String password)
{
    Serial.println(F("Opening  hostspot ..."));
    Serial.print(F("ap_ssid:"));
    Serial.println(ssid);
    Serial.print(F("ap_psk:"));
    Serial.println(password);

    WiFi.enableAP(true);
    WiFi.softAP(ssid.c_str(), password.c_str());
    apIP = WiFi.softAPIP().toString();
    return true;
}

String WiFiHelper::getAPIP()
{
    apIP = WiFi.softAPIP().toString();
    return apIP;
}

String WiFiHelper::getSTAIP()
{
    staIP = WiFi.localIP().toString();
    return staIP;
}

bool WiFiHelper::checkStatus()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        if (is_connected == true)
        {
            is_connected = false;
            Serial.println("[DISCONNECTED] wifi disconnected");
        }
    }
    return is_connected;
}
