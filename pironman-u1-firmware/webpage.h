#pragma once

#include <ESP.h>
#include <WiFi.h>
#include "debug.h"
#include "config.h"
#include "date_time.h"
#include "power.h"
#include "rgb_led.h"
#include "www_source/index_html.h"
#include "www_source/index_css.h"
#include "www_source/main_js.h"
#include "www_source/main_css.h"
#include "www_source/favicon_ico.h"
#include "www_source/webpage_static_data.h"
#include "www_source/asset_manifest.h"
// ----------------------------------------------------------------
#define WEB_PORT 34001
struct WebpageConfig
{
    String version;
    String (*onData)();
    bool (*onConfig)(int mode, bool enable, String ssid, String password);
};
// extern WebpageConfig webpageConfig;

void mDNSInit(String hostname);
void webPageBegin(WebpageConfig *config);
// void webPageBegin();
void webPageLoop();
void webOnData(String (*function)());
void webOnWifiConfig(bool (*function)(int, String, String));
void websetVersion(String version);