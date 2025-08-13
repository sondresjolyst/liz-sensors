// Copyright (c) 2023-2025 Sondre Sjølyst

#include "MQTTHelper.h"
#include <ArduinoJson.h>
#include <regex>
#include <string>

extern void checkSerialForCredentials();

const char *TOPIC_ROOT = "garge/devices/";
const char *SENSOR_TYPE_TEMPERATURE = "temperature";
const char *SENSOR_TYPE_HUMIDITY = "humidity";
const char *SENSOR_TYPE_VOLTAGE = "voltage";
const char *TOPIC_CONFIG = "/config";
const char *TOPIC_STATE = "/state";
const char *TOPIC_SET = "/set";

String getGargeDeviceNameUnderscore(const String &mac) {
  return String("garge_") + mac;
}

String getGargeDeviceNameSpace(const String &mac) {
  return String("garge ") + mac;
}

String getBaseTopic(const String &mac) {
  return String(TOPIC_ROOT) + getGargeDeviceNameUnderscore(mac) + "/";
}

String getSensorTopic(const String &mac, const char *type) {
  return getBaseTopic(mac) + getGargeDeviceNameUnderscore(mac) + "_" +
         String(type);
}

String getSensorConfigTopic(const String &mac, const char *type) {
  return getSensorTopic(mac, type) + TOPIC_CONFIG;
}

String getSensorStateTopic(const String &mac, const char *type) {
  return getSensorTopic(mac, type) + TOPIC_STATE;
}
String getDiscoveryTopic(const String &mac, const String &targetDeviceId) {
  return getBaseTopic(mac) + "discovered_devices/" + targetDeviceId +
         "/discovered";
}

String getDeviceSetTopic(const String &targetDeviceId) {
  return String(TOPIC_ROOT) + targetDeviceId + "/set";
}

void publishGargeSensorConfig(const String &mac, const char *type,
                              const char *unit, const char *devClass,
                              const char *valueTemplate) {
  String configTopic = getSensorConfigTopic(mac, type);
  String stateTopic = getSensorStateTopic(mac, type);

  DynamicJsonDocument doc(512);
  char buffer[512];

  doc["name"] = getGargeDeviceNameSpace(mac) + " " + type;
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = stateTopic;
  doc["unit_of_meas"] = unit;
  doc["dev_cla"] = devClass;
  doc["frc_upd"] = true;
  doc["uniq_id"] = getGargeDeviceNameUnderscore(mac) + "_" + type;
  doc["val_tpl"] = valueTemplate;
  doc["parent_name"] = getGargeDeviceNameUnderscore(mac);
  doc["version"] = VERSION;

  size_t n = serializeJson(doc, buffer);

  bool publish = mqttClient->publish(configTopic.c_str(),
                                     (const uint8_t *)buffer, n, true);

  printHelper.log("DEBUG", "Publishing config for %s: %s", configTopic.c_str(),
                  buffer);
  printHelper.log("INFO", "Publishing config for %s: %s", configTopic.c_str(),
                  publish ? "Success" : "Failed");
}

bool publishGargeSensorState(const String &mac, const char *type,
                             const String &payload) {
  String stateTopic = getSensorStateTopic(mac, type);
  bool publish = mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);

  printHelper.log("DEBUG", "Publishing state for %s: %s", stateTopic.c_str(),
                  payload.c_str());
  printHelper.log("INFO", "Publishing state for %s: %s", stateTopic.c_str(),
                  publish ? "Success" : "Failed");
  return publish;
}

void publishGargeDiscoveryEvent(const String &mac, const String &deviceName,
                                const String &type) {
  String discoveryTopic =
      getBaseTopic(mac) + "discovered_devices/" + deviceName + "/discovered";

  DynamicJsonDocument doc(256);
  doc["DiscoveredBy"] = getGargeDeviceNameUnderscore(mac);
  doc["Target"] = deviceName;
  doc["Type"] = type;
  char timeBuf[32];
  time_t now = time(nullptr);
  strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
  doc["Timestamp"] = timeBuf;

  char buffer[256];
  size_t n = serializeJson(doc, buffer);

  bool publish = mqttClient->publish(discoveryTopic.c_str(),
                                     (const uint8_t *)buffer, n, true);

  printHelper.log("INFO", "Published discovery event to %s: %s",
                  discoveryTopic.c_str(), publish ? "Success" : "Failed");
}

void publishDiscoveredDeviceConfig(const String &deviceName, const char *model,
                                   const char *manufacturer) {
  String configTopic = String(TOPIC_ROOT) + deviceName + "/config";
  String stateTopic = String(TOPIC_ROOT) + deviceName + "/state";
  String setTopic = String(TOPIC_ROOT) + deviceName + "/set";

  DynamicJsonDocument doc(1024);
  char buffer[1024];

  doc["name"] = deviceName;
  doc["command_topic"] = setTopic;
  doc["state_topic"] = stateTopic;
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["optimistic"] = false;
  doc["qos"] = 1;
  doc["retain"] = true;
  doc["uniq_id"] = deviceName;
  doc["device"]["identifiers"] = deviceName;
  doc["device"]["name"] = deviceName;
  doc["device"]["model"] = model;
  doc["device"]["manufacturer"] = manufacturer;

  size_t n = serializeJson(doc, buffer, sizeof(buffer));

  bool publish = mqttClient->publish(configTopic.c_str(),
                                     (const uint8_t *)buffer, n, true);

  printHelper.log("INFO", "Publishing discovered device config to %s: %s",
                  configTopic.c_str(), publish ? "Success" : "Failed");
}

void publishDiscoveredDeviceState(const String &mac, const String &deviceName,
                                  const String &payload) {
  String stateTopic = getBaseTopic(mac) + deviceName + TOPIC_STATE;

  bool publish = mqttClient->publish(stateTopic.c_str(), payload.c_str(), true);

  printHelper.log("DEBUG", "Publishing state for %s: %s", stateTopic.c_str(),
                  payload.c_str());
  printHelper.log("INFO", "Publishing state for %s: %s", stateTopic.c_str(),
                  publish ? "Success" : "Failed");
}

void publishDiscoveredWizState(const String &mac, const String &deviceName,
                               bool lightState) {
  printHelper.log("DEBUG", "publishDiscoveredWizState...");
  String payload = lightState ? "ON" : "OFF";
  publishDiscoveredDeviceState(mac, deviceName, payload);
  printHelper.log("DEBUG", "Device: %s, State: %s", deviceName.c_str(),
                  payload.c_str());
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  printHelper.log("INFO", "Message arrived [%s]", topic);

  String payloadStr;
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += static_cast<char>(payload[i]);
  }
  printHelper.log("DEBUG", "Payload: %s", payloadStr.c_str());

  std::string topicStr(topic);
  size_t lastSlash = topicStr.rfind('/');
  size_t secondLastSlash = topicStr.rfind('/', lastSlash - 1);
  std::string deviceNamePart =
      topicStr.substr(secondLastSlash + 1, lastSlash - secondLastSlash - 1);

  // deviceMac is always the last characters after the last '_'
  size_t lastUnderscore = deviceNamePart.rfind('_');
  std::string deviceMac = deviceNamePart.substr(lastUnderscore + 1);

  // Use regex to extract "SOCKET" or "SHRGBC" from deviceNamePart
  std::smatch match;
  std::string moduleType;
  if (std::regex_search(deviceNamePart, match, std::regex("(SOCKET|SHRGBC)"))) {
    moduleType = match.str();
  } else {
    moduleType = "unknown";
  }

  printHelper.log("DEBUG", "deviceNamePart: %s, deviceMac: %s, moduleType: %s",
                  deviceNamePart.c_str(), deviceMac.c_str(),
                  moduleType.c_str());

  // Find the device IP using the MAC address
  std::string deviceIP;
  for (const auto &device : liz::getDiscoveredDevices()) {
    if (std::get<1>(device) == deviceMac) {
      deviceIP = std::get<0>(device);
      break;
    }
  }

  printHelper.log("DEBUG", "Device IP: %s, Port: %d, Payload: %s",
                  deviceIP.c_str(), port, payloadStr.c_str());

  // If the payload is "on", turn on the light/switch
  if (payloadStr == "ON") {
    liz::setPilot(deviceIP.c_str(), port, true);
  } else if (payloadStr == "OFF") {
    liz::setPilot(deviceIP.c_str(), port, false);
  }

  auto response = liz::getPilot(deviceIP.c_str(), port);
  if (response) {
    printHelper.log("DEBUG", "Response: %s", response->c_str());

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response->c_str());
    if (error) {
      printHelper.log("ERROR", "Failed to parse JSON response");
    } else {
      bool state = doc["result"]["state"];

      std::string deviceName = "wiz_" + moduleType + "_" + deviceMac;

      printHelper.log("DEBUG",
                      "Device Name: %s, Module Name: %s, MAC: %s, State: %s",
                      deviceName.c_str(), moduleType.c_str(), deviceMac.c_str(),
                      state ? "true" : "false");
      publishDiscoveredWizState(CHIP_ID, deviceName.c_str(), state);
    }
  }
}

bool mqttStatus() { return mqttClient->connected(); }

void connectToMQTT() {
  printHelper.log("INFO", "Attempting to connect to MQTT broker: %s",
                  MQTT_BROKER);

  mqttClient->setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient->setBufferSize(1024);
  mqttClient->setCallback(mqttCallback);

  String lastUsername = EEPROM_MQTT_USERNAME;
  String lastPassword = EEPROM_MQTT_PASSWORD;

  while (!mqttClient->connected()) {
    liz::clearDiscoveredDevices();
    printHelper.log("INFO", "Clearing Discovered Devices");

    printHelper.log("DEBUG", "WiFi.status(): %d, IP: %s", WiFi.status(),
                    WiFi.localIP().toString().c_str());
    printHelper.log("DEBUG", "WiFi RSSI: %d", WiFi.RSSI());
    char errbuf[128];
    int errcode = secureClient->lastError(errbuf, sizeof(errbuf));
    printHelper.log("DEBUG",
                    "SecureClient connected(): %d, available(): %d, "
                    "lastError(): %d, msg: %s",
                    secureClient->connected(), secureClient->available(),
                    errcode, errbuf);

    printHelper.log("DEBUG", "Free heap: %u", ESP.getFreeHeap());

    if (EEPROM_MQTT_USERNAME != lastUsername ||
        EEPROM_MQTT_PASSWORD != lastPassword) {
      printHelper.log(
          "INFO",
          "Detected updated MQTT credentials, breaking out of connect loop.");
      break;
    }

    printHelper.log("DEBUG", "Calling mqttClient->connect()...");
    if (mqttClient->connect(CHIP_ID.c_str(), EEPROM_MQTT_USERNAME.c_str(),
                            EEPROM_MQTT_PASSWORD.c_str())) {
      printHelper.log("INFO", "MQTT connected");

      if (strcmp(GARGE_TYPE, "sensor") == 0) {
        publishGargeSensorConfig(
            CHIP_ID.c_str(), "temperature", "°C", "temperature",
            "{{value_json.temperature | round(3) | default(0)}}");
        publishGargeSensorConfig(
            CHIP_ID.c_str(), "humidity", "%", "humidity",
            "{{value_json.humidity | round(3) | default(0)}}");
      } else if (strcmp(GARGE_TYPE, "voltmeter") == 0) {
        publishGargeSensorConfig(
            CHIP_ID.c_str(), "voltage", "V", "voltage",
            "{{value_json.voltage | round(3) | default(0)}}");
      }
    } else {
      printHelper.log("ERROR", "MQTT connection failed! Error code = %d",
                      mqttClient->state());

      errcode = secureClient->lastError(errbuf, sizeof(errbuf));
      printHelper.log("DEBUG",
                      "SecureClient connected(): %d, lastError(): %d, msg: %s",
                      secureClient->connected(), errcode, errbuf);
      printHelper.log("DEBUG", "WiFi.status(): %d, IP: %s", WiFi.status(),
                      WiFi.localIP().toString().c_str());
    }

    int64_t waitStart = millis();
    while (millis() - waitStart < 5000) {
      checkSerialForCredentials();
      delay(10);
    }
  }
}
