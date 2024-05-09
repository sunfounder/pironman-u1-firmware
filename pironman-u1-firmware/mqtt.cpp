#include "mqtt.h"

char *mqttBroker;
int mqtttPort;
char *id;
char *mqttUsername;
char *mqttPassword;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void *callback(char *topic, byte *payload, unsigned int length);

void mqttClientStart(const char *broker, int port,
                     const char *username, const char *password,
                     void (*callback)(char *, uint8_t *, unsigned int))
{
    mqttClient.setServer(broker, port);
    mqttClient.setCallback(callback);
    // mqttClient.connect("ESP32-SPC", username, password);
    mqttClient.connect("ESP32-SPC");

    //
}
