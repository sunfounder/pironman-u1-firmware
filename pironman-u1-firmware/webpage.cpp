#include "webpage.h"

#define ESP_ASYNC_TCP_MAX_CONNECTIONS 1
#define ESP32
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

String _client_ip = "";
// ----------------------------------------------------------------
void mDNSInit(String hostname)
{
    info("mDNS start: %s\n", hostname.c_str());
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

void setSTASwitchHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    debug("setting STA switch\n");

    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("sta_switch"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    bool sw = doc["sta_switch"];
    doc.clear();

    // --- copy the config ---
    configPrefs.begin(CONFIG_PREFS_NAMESPACE);
    if (sw)
    {
        configPrefs.putUChar(STA_ENABLE_KEYNAME, 1);
    }
    else
    {
        configPrefs.putUChar(STA_ENABLE_KEYNAME, 0);
    }
    configPrefs.end();

    config.staENABLE = sw;

    info("config STA -> switch: %d\n", config.staENABLE);

    // ---  ---
    if (sw)
    {
        info("restart STA -> ssid: %s, psk: %s\n", config.staSSID, config.staPSK);
        WiFi.enableSTA(true);
        WiFi.begin(config.staSSID, config.staPSK);
    }
    else
    {
        info("close STA");
        WiFi.enableSTA(false);
    }

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setWiFiConfigHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    debug("setting STA wifi\n");

    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("sta_switch") ||
        !doc.containsKey("sta_ssid") ||
        !doc.containsKey("sta_psk"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    bool sw = doc["sta_switch"];
    String ssid = doc["sta_ssid"];
    String psk = doc["sta_psk"];

    doc.clear();

    // --- copy the config ---
    configPrefs.begin(CONFIG_PREFS_NAMESPACE);
    if (sw)
    {
        configPrefs.putUChar(STA_ENABLE_KEYNAME, 1);
    }
    else
    {
        configPrefs.putUChar(STA_ENABLE_KEYNAME, 0);
    }
    configPrefs.putString(STA_SSID_KEYNAME, ssid);
    configPrefs.putString(STA_PSK_KEYNAME, psk);
    configPrefs.end();

    config.staENABLE = sw;
    strlcpy(config.staSSID, ssid.c_str(), sizeof(config.staSSID));
    strlcpy(config.staPSK, psk.c_str(), sizeof(config.staPSK));

    info("config STA -> switch: %d, ssid: %s, psk: %s\n", config.staENABLE, config.staSSID, config.staPSK);

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setWiFiRestartHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    debug("restart STA wifi\n");

    // --- response before operation ---
    request->send_P(200, "application/json", okJson);
    delay(200);

    // --- operate  ---
    if (config.staENABLE)
    {
        info("restart STA -> ssid: %s, psk: %s\n", config.staSSID, config.staPSK);
        WiFi.enableSTA(true);
        WiFi.begin(config.staSSID, config.staPSK);
    }
    else
    {
        info("close STA");
        WiFi.enableSTA(false);
    }
}

void setAPConfigHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    debug("setting AP\n");

    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("ap_ssid") || !doc.containsKey("ap_psk"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    String ssid = doc["ap_ssid"];
    String psk = doc["ap_psk"];
    doc.clear();

    // --- copy the config ---
    configPrefs.begin(CONFIG_PREFS_NAMESPACE); // namespace
    configPrefs.putString(AP_SSID_KEYNAME, ssid);
    configPrefs.putString(AP_PSK_KEYNAME, psk);
    configPrefs.end();

    strlcpy(config.apSSID, ssid.c_str(), sizeof(config.apSSID));
    strlcpy(config.apPSK, psk.c_str(), sizeof(config.apPSK));

    info("config AP -> ssid: %s, psk: %s\n", config.apSSID, config.apPSK);

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setAPRestartHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    debug("restart AP\n");

    // --- response before operation ---
    request->send_P(200, "application/json", okJson);
    delay(200);

    // --- operate  ---
    info("setting AP -> ssid: %s, psk: %s\n", config.apSSID, config.apPSK);
    WiFi.enableAP(true);
    WiFi.softAP(config.apSSID, config.apPSK);
}

void setOutputHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    debug("setting Output\n");

    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("switch"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    uint8_t sw = doc["switch"];
    doc.clear();

    // --- operate  ---
    info("setting Output: %d\n", sw);
    switch (sw)
    {
    case 0:
        powerOutClose();
        break;
    case 1:
        requestShutDown(3); // web control shutdown request
        rgbMode = RGB_MODE_WAIT_SHUTDOWN;
        break;
    case 2:
        powerOutOpen();
        break;
    default:
        break;
    }

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setAutoTimeHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    debug("setting AutoTime\n");

    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("enable"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    bool _enable = doc["enable"];
    doc.clear();

    // --- copy the config ---
    configPrefs.begin(CONFIG_PREFS_NAMESPACE);
    if (_enable)
    {
        configPrefs.putUChar(AUTO_TIME_ENABLE_KEYNAME, 1);

        info("ntp time sync now (%s)", config.ntpServe);
        ntpTimeSync();
    }
    else
    {
        configPrefs.putUChar(AUTO_TIME_ENABLE_KEYNAME, 0);
    }

    config.autoTimeEnable = configPrefs.getUChar(AUTO_TIME_ENABLE_KEYNAME, DEFAULT_AUTO_TIME_ENABLE);
    configPrefs.end();

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setTimestampHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);
    if (!doc.containsKey("timestamp"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    uint64_t _timesample = doc["timestamp"];
    doc.clear();

    // --- operate  ---
    info("set Time: %d\n", _timesample);

    struct timeval tv;
    tv.tv_sec = _timesample;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setTimeZoneHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("timezone"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    String tz = doc["timezone"];
    doc.clear();

    // --- copy the config ---
    configPrefs.begin(CONFIG_PREFS_NAMESPACE); // namespace
    configPrefs.putString(TIME_ZONE_KEYNAME, tz);
    configPrefs.end();

    strlcpy(config.timezone, tz.c_str(), sizeof(config.timezone));

    // --- operate ---
    Serial.printf("set TimeZone: %s", config.timezone);
    setenv("TZ", config.timezone, 1);
    tzset();

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setNTPServerHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("ntp_server"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    String _ntp_server = doc["ntp_server"];
    doc.clear();

    // --- copy the config ---
    configPrefs.begin(CONFIG_PREFS_NAMESPACE); // namespace
    configPrefs.putString(NTP_SERVER_KEYNAME, _ntp_server);
    configPrefs.end();

    strlcpy(config.ntpServe, _ntp_server.c_str(), sizeof(config.ntpServe));

    // --- operate ---
    info("set ntp_server: %s", config.ntpServe);
    info("ntp time sync now (%s)", config.ntpServe);
    ntpTimeSync();

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

// -----------------------
extern uint8_t settingBuffer[];

void setFanPowerHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    debug("setting Fan Power\n");

    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("fan_mode"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    uint8_t _power = doc["fan_mode"];
    doc.clear();

    // --- operate  ---
    info("setting Fan Power: %d\n", _power);
    settingBuffer[0] = _power;

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setShutdownPctHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("shutdown-percentage"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    uint8_t pct = doc["shutdown-percentage"];
    doc.clear();

    // --- operate  ---
    if (pct > 100)
    {
        pct = 100;
    }
    Serial.printf("set shutdown-percentage: %d\n", pct);
    settingBuffer[9] = pct;

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

// -----------------------

void setSDDataIntervalHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("interval"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    uint32_t _interval = doc["interval"];
    doc.clear();

    // --- copy the config ---
    Serial.printf("set sd-data-interval: %d\n", _interval);

    configPrefs.begin(CONFIG_PREFS_NAMESPACE); // namespace
    configPrefs.putUInt(SD_DATA_INTERVAL_KEYNAME, _interval);
    configPrefs.end();

    config.sdDataInterval = _interval;

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void setSDDataRetainHandler(AsyncWebServerRequest *request, uint8_t *data)
{
    // --- convert into json ---
    DynamicJsonDocument doc(256);
    deserializeJson(doc, (const char *)data);

    if (!doc.containsKey("retain"))
    {
        doc.clear();
        request->send_P(200, "application/json", invalidKeyJson);
        return;
    }

    uint16_t _retain = doc["retain"];
    doc.clear();

    // --- copy the config ---
    Serial.printf("set sd-data-_retain: %d\n", _retain);

    configPrefs.begin(CONFIG_PREFS_NAMESPACE); // namespace
    configPrefs.putUInt(SD_DATA_RETAIN_KEYNAME, _retain);
    configPrefs.end();

    config.sdDataRetain = _retain;

    // ---- response ----
    request->send_P(200, "application/json", okJson);
}

void otaHandler(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        info("OTA ....");
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
            info("OTA completed");
        }
    }
}

void webPageBegin(WebpageConfig *webConfig)
{
    webpageConfig = webConfig;
    currentVersion = webpageConfig->version;
    user_onData = webpageConfig->onData;

    // if (!SPIFFS.begin())
    // {
    //     Serial.println("An Error has occurred while mounting SPIFFS");
    //     return;
    // }

    // ------------------ repsonse header -------------------
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "OPTIONS, GET, POST, PUT, DELETE");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

    // ------------------------ www ------------------------
    // index
    server.on("/", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  if (_client_ip != "")
                  {
                      request->send_P(200, "text/html", waitHtml);
                  }
                  else
                  {
                      AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
                      response->addHeader("Content-Encoding", "gzip");
                      response->addHeader("Cache-Control", "no-cache");
                      request->send(response);
                  }

                  //   request->send(SPIFFS, "/www/index.html", "text/html");
              });
    // css
    server.on("/index.css", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", index_css_gz, index_css_gz_len);
                  response->addHeader("Content-Encoding", "gzip");
                  response->addHeader("Cache-Control", "max-age=6000");
                  request->send(response);

                  //   request->send(SPIFFS, "/www/index.css", "text/css");
              });
#if 0
    server.on("/static/css/main.91c96fec.css", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", main_91c96fec_css_gz, main_91c96fec_css_gz_len);
                  response->addHeader("Content-Encoding", "gzip");
                  response->addHeader("Cache-Control", "max-age=6000");
                  request->send(response);

                  //   request->send(SPIFFS, "/www/styles.css", "text/css");
              });
#else
    server.on("/static/css/main.91c96fec.css", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginChunkedResponse(
                      "text/css",
                      [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                      {
                          //   debug("chunk max len: %d\n", maxLen);
                          uint32_t size = main_91c96fec_css_gz_len - index;
                          if (MAX_CHUNK_SIZE < maxLen)
                          {
                              if (size > MAX_CHUNK_SIZE)
                              {
                                  size = MAX_CHUNK_SIZE;
                              }
                          }
                          else
                          {
                              if (size > maxLen)
                              {
                                  size = maxLen;
                              }
                          }
                          //   debug("chunk send: %d\n", size);
                          memcpy(buffer, main_91c96fec_css_gz + index, size);

                          if (size == 0)
                          {
                              _client_ip = "";
                          }
                          return size;
                      });
                  response->addHeader("Content-Encoding", "gzip");
                  response->addHeader("Cache-Control", "max-age=6000");
                  request->send(response);
              });
#endif

// js
#if 0
    server.on("/static/js/main.335506f5.js", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", main_335506f5_js_gz, main_335506f5_js_gz_len);
                  response->addHeader("Content-Encoding", "gzip");
                  response->addHeader("Cache-Control", "max-age=6000");
                  request->send(response);

                  //   request->send(SPIFFS, "/www/script.js", "text/javascript");
              });
#else
    server.on("/static/js/main.335506f5.js", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  _client_ip = request->client()->remoteIP().toString();
                  debug("remoteIP: %s\n", _client_ip.c_str());

                  request->onDisconnect(
                      []()
                      {
                          _client_ip = "";
                      });

                  AsyncWebServerResponse *response = request->beginChunkedResponse(
                      "text/javascript",
                      [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                      {
                          //   debug("chunk max len: %d\n", maxLen);
                          uint32_t size = main_335506f5_js_gz_len - index;
                          if (MAX_CHUNK_SIZE < maxLen)
                          {
                              if (size > MAX_CHUNK_SIZE)
                              {
                                  size = MAX_CHUNK_SIZE;
                              }
                          }
                          else
                          {
                              if (size > maxLen)
                              {
                                  size = maxLen;
                              }
                          }
                          //   debug("chunk send: %d\n", size);
                          memcpy(buffer, main_335506f5_js_gz + index, size);

                          if (size == 0)
                          {
                              _client_ip = "";
                          }
                          return size;
                      });
                  response->addHeader("Content-Encoding", "gzip");
                  response->addHeader("Cache-Control", "max-age=6000");
                  request->send(response);
              });
#endif
    // favicon
    server.on("/favicon.ico", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon_ico_gz, favicon_ico_gz_len);
                  response->addHeader("Content-Encoding", "gzip");
                  response->addHeader("Cache-Control", "max-age=6000");
                  request->send(response);

                  //   request->send(SPIFFS, "/www/favicon.ico", "text/plain");
              });

    // ------------------------ API ------------------------
    // NotAPIFound
    server.onNotFound(
        [](AsyncWebServerRequest *request)
        {
            // String url = request->url();
            // Serial.printf("APINotFound: %s\n", url);
            request->send_P(200, "application/json", apiNotFoundJson);
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
                  request->send_P(200, "application/json", deviceInfoJson);
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
                  json += "\"shutdown_percentage\":" + String(config.shutdownPct);
                  json += ",\"fan_power\":" + String(config.fanPower);
                  json += ",\"timestamp\":" + String(getTimeSample());
                  json += ",\"timezone\":\"" + String(config.timezone) + "\"";
                  json += ",\"auto_time_switch\":" + String(config.autoTimeEnable ? "true" : "false");
                  json += ",\"ntp_server\":\"" + String(config.ntpServe) + "\"";
                  json += ",\"mac_address\":\"" + String(WiFi.macAddress()) + "\"";
                  json += ",\"ip_address\":\"" + String(WiFiHelper::is_connected ? WiFi.localIP().toString() : "") + "\"";
                  json += ",\"sd_card_used\":" + String(SD.usedBytes() / (1024 * 1024));
                  json += ",\"sd_card_total\":" + String(SD.totalBytes() / (1024 * 1024));
                  json += ",\"sd_card_data_interval\":" + String(config.sdDataInterval);
                  json += ",\"sd_card_data_retain\":" + String(config.sdDataRetain);
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
                  request->send_P(200, "application/json", startScanJson);
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
                      request->send_P(200, "application/json", noscanningJson);
                  }
                  else if (n == -1)
                  {
                      Serial.println("scanning");
                      request->send_P(200, "application/json", scanningJson);
                  }
                  else if (0)
                  {
                      Serial.println("no WiFi found");
                      request->send_P(200, "application/json", noWiFiFoundJson);
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
                  json += ",\"data\":\"" + String(WiFiHelper::is_connected ? WiFi.localIP().toString() : "") + "\"";
                  json += "}";

                  request->send(200, "application/json", json);
              });
    // get-default-on
    server.on("/api/v1.0/get-default-on", HTTP_GET,
              [](AsyncWebServerRequest *request)
              {
                  String json = "{";
                  json += "\"status\":true";
                  json += ",\"data\":" + String(checkDefaultOn() ? "true" : "false");
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
                debug("[REQUEST][set-config] %s\n", (const char *)data);
            }
            // set-output
            else if (request->url() == "/api/v1.0/set-output")
            {
                debug("[REQUEST][set-output] %s\n", (const char *)data);
                setOutputHandler(request, data);
            }
            // set-sta-switch
            else if (request->url() == "/api/v1.0/set-sta-switch")
            {
                debug("[REQUEST][set-sta-switch] %s\n", (const char *)data);
                setSTASwitchHandler(request, data);
            }
            // set-wifi-config
            else if (request->url() == "/api/v1.0/set-wifi-config")
            {
                debug("[REQUEST][set-wifi-config] %s\n", (const char *)data);
                setWiFiConfigHandler(request, data);
            }
            // set-wifi-restart
            else if (request->url() == "/api/v1.0/set-wifi-restart")
            {
                debug("[REQUEST][set-wifi-restart] %s\n", (const char *)data);
                setWiFiRestartHandler(request, data);
            }
            // set-ap-config
            else if (request->url() == "/api/v1.0/set-ap-config")
            {
                debug("[REQUEST][set-ap-config] %s\n", (const char *)data);
                setAPConfigHandler(request, data);
            }
            // set-ap-restart
            else if (request->url() == "/api/v1.0/set-ap-restart")
            {
                debug("[REQUEST][set-ap-restart] %s\n", (const char *)data);
                setAPRestartHandler(request, data);
            }
            // set-shutdown-percentage
            else if (request->url() == "/api/v1.0/set-shutdown-percentage")
            {
                debug("[REQUEST][set-shutdown-percentage] %s\n", (const char *)data);
                setShutdownPctHandler(request, data);
            }
            // set-auto-time
            else if (request->url() == "/api/v1.0/set-auto-time")
            {
                debug("[REQUEST][set-auto-time] %s\n", (const char *)data);
                setAutoTimeHandler(request, data);
            }
            // set-timestamp
            else if (request->url() == "/api/v1.0/set-timestamp")
            {
                debug("[REQUEST][set-timestamp] %s\n", (const char *)data);
                setTimestampHandler(request, data);
            }
            // set-timezone
            else if (request->url() == "/api/v1.0/set-timezone")
            {
                debug("[REQUEST][set-timezone] %s\n", (const char *)data);
                setTimeZoneHandler(request, data);
            }
            // set-ntp-server
            else if (request->url() == "/api/v1.0/set-ntp-server")
            {
                debug("[REQUEST][set-ntp-server] %s\n", (const char *)data);
                setNTPServerHandler(request, data);
            }
            // set-restart
            else if (request->url() == "/api/v1.0/set-restart")
            {
                debug("[REQUEST][set-restart] %s\n", (const char *)data);
                request->send_P(200, "application/json", okJson);
                delay(100);
                debug("restart now\n");
                ESP.restart();
            }
            // set-fan-power
            else if (request->url() == "/api/v1.0/set-fan-power")
            {
                debug("[REQUEST][set-fan-power] %s\n", (const char *)data);
                setFanPowerHandler(request, data);
            }
            else if (request->url() == "/api/v1.0/set-sd-data-interval")
            {
                debug("[REQUEST][set-sd-data-interval] %s\n", (const char *)data);
                setSDDataIntervalHandler(request, data);
            }
            else if (request->url() == "/api/v1.0/set-sd-data-retain")
            {
                debug("[REQUEST][set-sd-data-retain] %s\n", (const char *)data);
                setSDDataRetainHandler(request, data);
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
            }
        },
        [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
           size_t len, bool final)
        {
            otaHandler(request, filename, index, data, len, final);
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
