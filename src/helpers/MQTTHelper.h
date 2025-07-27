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

extern String CHIP_ID_STRING;
extern String MQTT_STATETOPIC;
extern const char *MQTT_BROKER;
extern const char *MQTT_HOSTNAME;
extern String EEPROM_MQTT_USERNAME;
extern String EEPROM_MQTT_PASSWORD;
extern const int MQTT_PORT;
extern const int port;

extern std::vector<std::pair<String, String>> discoveredDevices;
extern PRINTHelper printHelper;
extern WiFiClientSecure* secureClient;
extern PubSubClient* mqttClient;

void sendMQTTTemperatureDiscoveryMsg(String MQTT_STATETOPIC,
                                     String MQTT_HOSTNAME);
void sendMQTTHumidityDiscoveryMsg(String MQTT_STATETOPIC, String MQTT_HOSTNAME);
void sendMQTTVoltageDiscoveryMsg(String MQTT_STATETOPIC, String MQTT_HOSTNAME);
void sendMQTTWizDiscoveryMsg(std::string deviceIP, std::string deviceName);
void publishWizState(String deviceName, bool lightState);
void mqttCallback(char *topic, byte *payload, unsigned int length);
bool mqttStatus();
void connectToMQTT();

#endif  // SRC_HELPERS_MQTTHELPER_H_
