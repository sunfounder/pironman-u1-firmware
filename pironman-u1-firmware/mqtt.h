#pragma once

#include <WiFi.h>
#include "PubSubClient.h"

extern PubSubClient mqttClient;

void mqttClientStart(const char *broker, int port,
                     const char *username, const char *password,
                     void (*callback)(char *, uint8_t *, unsigned int));