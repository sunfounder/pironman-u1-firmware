#pragma once
#include <ESP.h>

static const char okJson[] PROGMEM = R"({"status":true,"data":"OK"})";
static const char nullJson[] PROGMEM = R"({"status":true,"data":[]})";
static const char apiNotFoundJson[] PROGMEM = R"({"status":false,"error":"APINotFound"})";
static const char startScanJson[] PROGMEM = R"({"status":true,"data":"start scanning"})";
static const char scanningJson[] PROGMEM = R"({"status":true,"data":"scaning"})";
static const char noscanningJson[] PROGMEM = R"({"status":true,"data":-2})";
static const char noWiFiFoundJson[] PROGMEM = R"({"status":true,"data":"no WiFi found"})";
static const char WiFiConnectingJson[] PROGMEM = R"({"status":true,"data":"WiFi Connecting"})";

static const char deviceInfoJson[] PROGMEM = R"({
"status": true,
"data": {
    "name": "Pironman U1",
    "id": "pironman-u1",
    "peripherals": [
        "history",
        "input_voltage",
        "input_current",
        "input_power",
        "is_input_plugged_in",
        "output_switch",
        "output_voltage",
        "output_current",
        "output_power",
        "power_source",
        "battery_voltage",
        "battery_current",
        "battery_power",
        "battery_capacity",
        "battery_percentage",
        "is_battery_plugged_in",
        "is_charging",
        "spc_fan_power",
        "sta_switch",
        "sta_ssid_scan",
        "sta_ssid",
        "sta_psk",
        "ap_ssid",
        "ap_psk",
        "download_history_file",
        "shutdown_percentage",
        "power_off_percentage",
        "shutdown_request",
        "time",
        "timezone",
        "auto_time_enable",
        "mac_address",
        "ip_address",
        "sd_card_usage",
        "ota_auto",
        "ota_manual",
        "mqtt"
    ]
}
})";
