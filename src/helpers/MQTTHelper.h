// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_MQTTHELPER_H_
#define SRC_HELPERS_MQTTHELPER_H_

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <regex>
#include <string>
#include <vector>

#include "../lib/liz/src/liz.h"
#include "PRINTHelper.h"

extern String CHIP_ID_STRING;
extern String MQTT_STATETOPIC;
extern String MQTT_HOSTNAME_STRING;
extern const char *MQTT_BROKER;
extern const char *MQTT_HOSTNAME;
extern const char *MQTT_PASS;
extern const char *MQTT_USER;
extern const int MQTT_PORT;
extern const int port;

extern std::vector<std::pair<String, String>> discoveredDevices;
extern PRINTHelper printHelper;
extern WiFiClient espClient;
extern PubSubClient client;

void sendMQTTTemperatureDiscoveryMsg(const String& discoveredDeviceId, const String& MQTT_STATETOPIC);
void sendMQTTHumidityDiscoveryMsg(const String& discoveredDeviceId, const String& MQTT_STATETOPIC);
void sendMQTTVoltageDiscoveryMsg(const String& discoveredDeviceId, const String& MQTT_STATETOPIC);
void sendMQTTSocketDiscoveryMsg(
    const std::string& deviceIP,
    const std::string& deviceName,
    const std::string& model = "Smart Socket",
    const std::string& manufacturer = "Generic"
);
void publishSocketState(const String& deviceName, bool socketState);
void mqttCallback(char *topic, byte *payload, unsigned int length);
bool mqttStatus();
void connectToMQTT();

#endif  // SRC_HELPERS_MQTTHELPER_H_
