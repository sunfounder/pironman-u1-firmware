
#include "wifi_helper.h"

bool WiFiHelper::is_connected = false;
uint8_t WiFiHelper::staDisconnectReason = 0;

void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.print("WiFi connected: ");
    Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
    WiFiHelper::is_connected = true;
}

void onSTADisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    WiFiHelper::staDisconnectReason = info.wifi_sta_disconnected.reason;
    Serial.printf("STA disconnected: %d->%s\n",
                  WiFiHelper::staDisconnectReason,
                  WiFi.disconnectReasonName((wifi_err_reason_t)WiFiHelper::staDisconnectReason));
    WiFiHelper::is_connected = false;
}

WiFiHelper::WiFiHelper()
{
    WiFi.setAutoReconnect(false);
    WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(onSTADisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
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
    Serial.print("STA Connecting.");
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
        if (WiFiHelper::is_connected == true)
        {
            WiFiHelper::is_connected = false;
            Serial.println("[DISCONNECTED] wifi disconnected");
        }
    }
    return WiFiHelper::is_connected;
}
