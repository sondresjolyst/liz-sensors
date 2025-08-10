// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_MQTTHELPER_H_
#define SRC_HELPERS_MQTTHELPER_H_

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <regex>
#include <string>
#include <vector>

#include "../lib/liz/src/liz.h"
#include "PRINTHelper.h"

extern String CHIP_ID;
extern const char *MQTT_BROKER;
extern String EEPROM_MQTT_USERNAME;
extern String EEPROM_MQTT_PASSWORD;
extern const int MQTT_PORT;
extern const int port;

extern std::vector<std::pair<String, String>> discoveredDevices;
extern PRINTHelper printHelper;
extern WiFiClientSecure *secureClient;
extern PubSubClient *mqttClient;

String getGargeDeviceNameUnderscore(const String &mac);

void publishGargeSensorConfig(const String &mac, const char *type,
                              const char *unit, const char *devClass,
                              const char *valueTemplate);
bool publishGargeSensorState(const String &mac, const char *type,
                             const String &payload);
void publishDiscoveredDeviceConfig(const String &deviceName, const char *model,
                                   const char *manufacturer);
void publishDiscoveredDeviceState(const String &mac, const String &deviceName,
                                  const String &payload);
void publishDiscoveredWizState(const String &mac, const String &deviceName,
                               bool lightState);
void publishGargeDiscoveryEvent(const String &mac, const String &deviceName,
                                const String &type);

void mqttCallback(char *topic, byte *payload, unsigned int length);
bool mqttStatus();
void connectToMQTT();

#endif  // SRC_HELPERS_MQTTHELPER_H_
