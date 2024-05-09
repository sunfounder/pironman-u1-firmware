#include "webpage.h"

/*
https://github.com/me-no-dev/ESPAsyncWebServer
*/
#include <ESPAsyncWebSrv.h>
// #include <ESPAsyncWebServer.h>

#include <ESPmDNS.h>
#include <Update.h>
#include <StreamString.h>
#include "wifi_helper.h"
#include <ArduinoJson.h>

#include "FS.h"
// #include "SPIFFS.h"
#include "LittleFS.h"

#define SPIFFS LittleFS

#include "SD.h"

/* This examples uses "quick re-define" of SPIFFS to run
   an existing sketch with LittleFS instead of SPIFFS

   You only need to format LittleFS the first time you run a
   test or else use the LittleFS plugin to create a partition
   https://github.com/lorol/arduino-esp32littlefs-plugin */

// #define FORMAT_LITTLEFS_IF_FAILED true

AsyncWebServer server(WEB_PORT);

WebpageConfig *webpageConfig;

String currentVersion = "";
String (*user_onData)();

String _updaterError;
size_t content_len;
WiFiHelper mywifi;

// ----------------------------------------------------------------
void mDNSInit(String hostname)
{
    MDNS.begin(hostname);
}

void _setUpdaterError()
{
    Serial.print("[Update Error]:");
    Update.printError(Serial);
    StreamString str;
    Update.printError(str);
    _updaterError = str.c_str();
}

void setWiFiConfigHandle(AsyncWebServerRequest *request, uint8_t *data)
{
    Serial.println("setting STA wifi");

    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);
    bool sw = doc["sta_switch"];
    String ssid = doc["sta_ssid"];
    String psk = doc["sta_psk"];

    doc.clear();

    // --- copy the config ---
    prefs.begin(PREFS_NAMESPACE);
    if (sw)
    {
        prefs.putUChar(STA_ENABLE_KEYNAME, 1);
    }
    else
    {
        prefs.putUChar(STA_ENABLE_KEYNAME, 0);
    }
    prefs.putString(STA_SSID_KEYNAME, ssid);
    prefs.putString(STA_PSK_KEYNAME, psk);
    prefs.end();

    config.staENABLE = sw;
    strlcpy(config.staSSID, ssid.c_str(), sizeof(config.staSSID));
    strlcpy(config.staPSK, psk.c_str(), sizeof(config.staPSK));

    Serial.printf("config STA -> switch: %d, ssid: %s, psk: %s\n", config.staENABLE, config.staSSID, config.staPSK);

    request->send_P(200, "application/json", okJson);
}

void setWiFiRestartHandle(AsyncWebServerRequest *request, uint8_t *data)
{
    Serial.println("restart STA wifi");

    Serial.printf("heap free: %d", ESP.getFreeHeap());

    // response before operation
    request->send_P(200, "application/json", okJson);
    delay(200);

    // --- operate  ---
    if (config.staENABLE)
    {
        Serial.printf("restart STA -> ssid: %s, psk: %s\n", config.staSSID, config.staPSK);
        WiFi.begin(config.staSSID, config.staPSK);
    }
    else
    {
        Serial.println("close STA");
        WiFi.enableSTA(false);
    }
}

void setAPConfigHandle(AsyncWebServerRequest *request, uint8_t *data)
{
    Serial.println("setting AP");

    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);
    String ssid = doc["ap_ssid"];
    String psk = doc["ap_psk"];
    doc.clear();
    // --- copy the config ---
    prefs.begin(PREFS_NAMESPACE); // namespace
    prefs.putString(AP_SSID_KEYNAME, ssid);
    prefs.putString(AP_PSK_KEYNAME, psk);
    prefs.end();

    strlcpy(config.apSSID, ssid.c_str(), sizeof(config.apSSID));
    strlcpy(config.apPSK, psk.c_str(), sizeof(config.apPSK));

    Serial.printf("config AP -> ssid: %s, psk: %s\n", config.apSSID, config.apPSK);

    request->send_P(200, "application/json", okJson);
}

void setAPRestartHandle(AsyncWebServerRequest *request, uint8_t *data)
{
    Serial.println("restart AP");

    // --- operate  ---
    Serial.printf("setting AP -> ssid: %s, psk: %s\n", config.apSSID, config.apPSK);
    WiFi.enableAP(true);
    WiFi.softAP(config.apSSID, config.apPSK);
}

void setOutputHandle(AsyncWebServerRequest *request, uint8_t *data)
{
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);
    uint8_t sw = doc["switch"];
    doc.clear();

    Serial.printf("setting Output: %d\n", sw);
    switch (sw)
    {
    case 0:
        requestShutDown(3); // web control shutdown request
        rgbMode = RGB_MODE_WAIT_SHUTDOWN;
        break;
    case 1:
        powerOutClose();
        break;
    case 2:
        powerOutOpen();
        break;
    default:
        break;
    }

    request->send_P(200, "application/json", okJson);
}

void setTimeSyncHandle(AsyncWebServerRequest *request, uint8_t *data)
{
    request->send_P(200, "application/json", okJson);
}

// ----------------------------------------------------------------
extern uint8_t settingBuffer[];

void setShutdownPctHandle(AsyncWebServerRequest *request, uint8_t *data)
{
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);
    uint8_t pct = doc["shutdown-percentage"];
    doc.clear();

    if (pct > 100)
    {
        pct = 100;
    }
    Serial.printf("sett shutdown-percentage: %d\n", pct);
    settingBuffer[9] = pct;

    request->send_P(200, "application/json", okJson);
}

void setPoweroffPctHandle(AsyncWebServerRequest *request, uint8_t *data)
{
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);
    uint8_t pct = doc["shutdown-percentage"];
    doc.clear();

    if (pct > 100)
    {
        pct = 100;
    }
    Serial.printf("sett shutdown-percentage: %d\n", pct);
    settingBuffer[9] = pct;

    request->send_P(200, "application/json", okJson);
}

void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        Serial.println("Update");
        content_len = request->contentLength();
        // if filename includes spiffs, update the spiffs partition
        if (filename.indexOf("filesystem") > -1)
        {
            if (!Update.begin(SPIFFS.totalBytes(), U_SPIFFS))
            { // start with max available size
                Serial.print("[Update SPIFFS Error]:");
                Update.printError(Serial);
            }
        }
        else
        {
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace, U_FLASH))
            { // start with max available size
                _setUpdaterError();
            }
        }
    }

    if (Update.write(data, len) != len)
    {
        _setUpdaterError();
    }

    if (final)
    {
        if (!Update.end(true))
        {
            _setUpdaterError();
        }
        else
        {
            Serial.println("Update complete");
        }
    }
}

// void webPageBegin(uint32_t *data_merge_fuction)
// void webPageBegin()
void webPageBegin(WebpageConfig *webConfig)
{
    webpageConfig = webConfig;

    // Serial.printf("webpageConfig pointer: %p\n", webpageConfig);
    // Serial.printf("onData pointer: %p\n", webpageConfig->onData);
    // Serial.printf("user_onData pointer: %p\n", user_onData);
    // Serial.printf("version2: %s\n", webpageConfig->version);

    currentVersion = webpageConfig->version;
    user_onData = webpageConfig->onData;

    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        // return;
    }

    // ------------------ repsonse header -------------------
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "OPTIONS, GET, POST, PUT, DELETE");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

    // ------------------------ www ------------------------
    // index
    server.on("/", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  Serial.println("server.on /");
                  request->send(200, "text/html", index_html);
                  //   request->send(SPIFFS, "/www/spc_dashboard.html", "text/html");
              });
    // css
    server.on("/index.css", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  request->send(200, "text/css", index_css);
                  //   request->send(SPIFFS, "/www/styles.css", "text/css");
              });
    server.on("/static/css/main.css", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", main_css_gz, main_css_gz_len);
                  response->addHeader("Content-Encoding", "gzip");
                  request->send(response);

                  //   request->send(SPIFFS, "/www/styles.css", "text/css");
              });
    // js
    server.on("/static/js/main.js", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", main_js_gz, main_js_gz_len);
                  response->addHeader("Content-Encoding", "gzip");
                  request->send(response);

                  //   request->send(SPIFFS, "/www/script.js", "text/javascript");
              });
    // ico
    server.on("/favicon.ico", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  request->send(200, "text/plain", "null");
                  //   request->send(SPIFFS, "/www/favicon.ico", "text/plain");
              });

    // ------------------------ API ------------------------
    // NotAPIFound
    server.onNotFound(
        [](AsyncWebServerRequest *request)
        {
            // String url = request->url();
            // Serial.printf("APINotFound: %s\n", url);
            request->send(200, "application/json", apiNotFoundJson);
        });

    // version
    server.on("/api/v1.0/get-version", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  json += "\"status\":true";
                  json += ",\"data\":\"" + currentVersion + "\"";
                  json += "}";

                  request->send(200, "application/json", json);
              });

    // get-device-info
    server.on("/api/v1.0/get-device-info", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  //   Serial.println("get-device-info");

                  request->send(200, "application/json", deviceInfoJson);
                  //   request->send(SPIFFS, "/device_info.json", "application/json");
              });

    // test
    server.on("/api/v1.0/test", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  request->send_P(200, "application/json", okJson);
              });

    // get-config
    server.on("/api/v1.0/get-config", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  json += "\"status\":true";
                  json += ",\"data\":{";
                  json += "\"wifi\":{";
                  json += "\"sta_switch\":" + String(config.staENABLE ? "true" : "false");
                  json += ",\"sta_ssid\":\"" + String(config.staSSID) + "\"";
                  json += "}";
                  json += ",\"system\":{";
                  json += "\"shutdown_percentage\":" + String(20);
                  json += ",\"power_off_percentage\":" + String(10);
                  json += ",\"timestamp\":" + String(getTimeSample());
                  json += ",\"timezone\":\"" + String(config.timezone) + "\"";
                  json += ",\"auto_time_switch\":true";
                  json += ",\"ntp_server\":\"" + String(config.ntpServe) + "\"";
                  json += ",\"mac_address\":\"" + String(WiFi.macAddress()) + "\"";
                  json += ",\"ip_address\":\"" + WiFi.localIP().toString() + "\"";
                  json += ",\"sd_card_used\":" + String(SD.usedBytes() / (1024 * 1024));
                  json += ",\"sd_card_total\":" + String(SD.totalBytes() / (1024 * 1024));
                  json += "}";
                  json += ",\"mqtt\":{";
                  json += "\"host\":\"" + String("pironman-U1-mqtt") + "\"";
                  json += ",\"port\":" + String(1883);
                  json += ",\"username\":\"" + String("mqtt") + "\"";
                  json += ",\"username\":\"" + String("mqtt") + "\"";
                  json += "}";
                  json += "}}";

                  request->send(200, "application/json", json);
                  //   request->send(SPIFFS, "/config.json", "application/json");
              });

    // get-history
    server.on("/api/v1.0/get-history", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  //   Serial.println("get-history");
                  request->send(200, "application/json", user_onData());
              });

    // get-history-file
    server.on("/api/v1.0/get-history-file", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  int paramsCount = request->params();
                  for (int i = 0; i < paramsCount; i++)
                  {
                      AsyncWebParameter *param = request->getParam(i);
                      if (param->isFile())
                      {
                          Serial.printf("Parameter[%s]: File\n", param->name().c_str());
                      }
                      else if (param->isPost())
                      {
                          Serial.printf("Parameter[%s]: Post\n", param->name().c_str());
                      }
                      else
                      {
                          Serial.printf("Parameter[%s]: %s\n", param->name().c_str(), param->value().c_str());
                      }
                  }

                  //   request->send(SD, "/history/2024-04/2024-04-22.csv", "text/csv");
                  request->send(SD, "/history/2024-04/2024-04-22.csv", "text/plain");
              });

    // get-time-range

    // get-wifi-config
    server.on("/api/v1.0/get-wifi-config", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  json += "\"status\":true";
                  json += ",\"data\":{";
                  json += "\"sta_switch\":" + String(config.staENABLE ? "true" : "false");
                  json += ",\"sta_ssid\":\"" + String(config.staSSID) + "\"";
                  //   json += ",\"sta_psk\":\"" + String(config.staPSK) + "\"";
                  json += "}}";

                  request->send(200, "application/json", json);
              });

    // start-wifi-scan
    server.on("/api/v1.0/start-wifi-scan", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  WiFi.scanNetworks(true);
                  request->send(200, "application/json", startScanJson);
              });

    // get-wifi-scan
    server.on("/api/v1.0/get-wifi-scan", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  int n = WiFi.scanComplete();
                  if (n == -2)
                  {
                      Serial.println("no scanning");
                      //   WiFi.scanNetworks(true);
                      request->send(200, "application/json", noscanningJson);
                  }
                  else if (n == -1)
                  {
                      Serial.println("scanning");
                      request->send(200, "application/json", scanningJson);
                  }
                  else if (0)
                  {
                      Serial.println("no WiFi found");
                      request->send(200, "application/json", noWiFiFoundJson);
                  }
                  else if (n)
                  {
                      Serial.printf("%d WiFi found\n", n);
                      String json = "{";
                      json += "\"status\":true";
                      json += ",\"data\":[";

                      for (int i = 0; i < n; ++i)
                      {
                          if (i)
                          {
                              json += ",";
                          }
                          json += "{";
                          json += "\"rssi\":" + String(WiFi.RSSI(i));
                          json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
                          json += ",\"bssid\":\"" + WiFi.BSSIDstr(i) + "\"";
                          json += ",\"channel\":" + String(WiFi.channel(i));
                          json += ",\"secure\":" + String(WiFi.encryptionType(i));
                          json += "}";
                      }

                      json += "]}";

                      WiFi.scanDelete();
                      request->send(200, "application/json", json);
                  }
              });

    // get-ap-config
    server.on("/api/v1.0/get-ap-config", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  json += "\"status\":true";
                  json += ",\"data\":{";
                  json += "\"ap_ssid\":\"" + String(config.apSSID) + "\"";
                  json += ",\"ap_psk\":\"" + String(config.apPSK) + "\"";
                  json += "}}";

                  request->send(200, "application/json", json);
              });

    // get-timestamp
    server.on("/api/v1.0/get-timestamp", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  json += "\"status\":true";
                  json += ",\"data\":" + String(getTimeSample());
                  json += "}";

                  request->send(200, "application/json", json);
              });

    // get-wifi-status
    server.on("/api/v1.0/get-wifi-status", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  json += "\"status\":true";
                  json += ",\"data\":" + String(WiFi.status());
                  json += "}";

                  request->send(200, "application/json", json);
              });

    // get-wifi-ip
    server.on("/api/v1.0/get-wifi-ip", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  json += "\"status\":true";
                  json += ",\"data\":\"" + WiFi.localIP().toString() + "\"";
                  json += "}";

                  request->send(200, "application/json", json);
              });

    // ----- RequestBody (client post) -----
    server.onRequestBody(
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            // set-config
            if (request->url() == "/api/v1.0/set-config")
            {
                Serial.printf("[REQUEST][set-config] %s\n", (const char *)data);
            }
            // set-output
            else if (request->url() == "/api/v1.0/set-output")
            {
                Serial.printf("[REQUEST][set-output] %s\n", (const char *)data);
                setOutputHandle(request, data);
            }
            // set-wifi-config
            else if (request->url() == "/api/v1.0/set-wifi-config")
            {
                Serial.printf("[REQUEST][set-wifi-config] %s\n", (const char *)data);
                setWiFiConfigHandle(request, data);
            }
            // set-wifi-restart
            else if (request->url() == "/api/v1.0/set-wifi-restart")
            {
                Serial.printf("[REQUEST][set-wifi-restart] %s\n", (const char *)data);
                setWiFiRestartHandle(request, data);
            }
            // set-ap-config
            else if (request->url() == "/api/v1.0/set-ap-config")
            {
                Serial.printf("[REQUEST][set-ap-config] %s\n", (const char *)data);
                setAPConfigHandle(request, data);
            }
            // set-ap-restart
            else if (request->url() == "/api/v1.0/set-ap-restart")
            {
                Serial.printf("[REQUEST][set-ap-restart] %s\n", (const char *)data);
                setAPRestartHandle(request, data);
            }
            // set-time-sync
            else if (request->url() == "/api/v1.0/set-time-sync")
            {
                Serial.printf("[REQUEST][set-time-sync] %s\n", (const char *)data);
                setTimeSyncHandle(request, data);
            }
            // set-shutdown-percentage
            else if (request->url() == "/api/v1.0/set-shutdown-percentage")
            {
                Serial.printf("[REQUEST][set-time-sync] %s\n", (const char *)data);
                setShutdownPctHandle(request, data);
            }
            // set-power-off-percentage
            else if (request->url() == "/api/v1.0/set-power-off-percentage")
            {
                Serial.printf("[REQUEST][set-time-sync] %s\n", (const char *)data);
                setPoweroffPctHandle(request, data);
            }
        });

    // ota-update
    server.on(
        "/api/v1.0/ota-update", HTTP_POST,
        [](AsyncWebServerRequest *request)
        {
            if (Update.hasError())
            {
                String json = "{";
                json += "\"status\":true";
                json += ",\"data\":\"error\":\"" + String(_updaterError) + "\"";
                json += "}";
                request->send(200, F("application/json"), json);
            }
            else
            {
                request->send_P(200, "application/json", okJson);
                delay(100);
                ESP.restart();
            }
        },
        [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
           size_t len, bool final)
        {
            handleDoUpdate(request, filename, index, data, len, final);
        });

    // ---------------- begin ----------------
    server.begin();

    Serial.printf("Webpage on: \n");
    Serial.printf("    %s.local:%d\n", config.hostname, WEB_PORT);
    if (WiFi.localIP().toString() != "")
    {
        Serial.printf("    %s:%d\n", WiFi.localIP().toString().c_str(), WEB_PORT);
    }
    Serial.printf("    %s:%d\n", WiFi.softAPIP().toString().c_str(), WEB_PORT);
}

void webPageLoop()
{
    // server.handleClient();
}