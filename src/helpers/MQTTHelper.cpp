// Copyright (c) 2023-2025 Sondre Sjølyst

#include <string>

#include "LogHelper.h"
#include "MQTTHelper.h"

extern void checkSerialForCredentials();

void sendMQTTTemperatureDiscoveryMsg(String MQTT_STATETOPIC,
                                     String MQTT_HOSTNAME) {
  LOG("DEBUG", "sendMQTTTemperatureDiscoveryMsg...");

  String discoveryTopic =
      "homeassistant/sensor/" + String(MQTT_HOSTNAME) + "_temperature/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = "Garge " + CHIP_ID_STRING + " Temperature";
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = MQTT_STATETOPIC;
  doc["unit_of_meas"] = "°C";
  doc["dev_cla"] = "temperature";
  doc["frc_upd"] = true;
  doc["uniq_id"] = String(MQTT_HOSTNAME) + "_temperature";
  doc["val_tpl"] = "{{value_json.temperature | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  LOG("DEBUG", "Serialized JSON size: %d", n);
  LOG("DEBUG", "MQTT buffer size: %d", mqttClient->getBufferSize());

  bool ok = mqttClient->publish(discoveryTopic.c_str(), buffer, n);
  if (!ok) {
    LOG("ERROR",
        "Failed to publish discovery message! (PubSubClient returned false)");
  }
  LOG("DEBUG", "sendMQTTTemperatureDiscoveryMsg done.");
}

void sendMQTTHumidityDiscoveryMsg(String MQTT_STATETOPIC,
                                  String MQTT_HOSTNAME) {
  String discoveryTopic =
      "homeassistant/sensor/" + String(MQTT_HOSTNAME) + "_humidity/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = "Garge " + CHIP_ID_STRING + " Humidity";
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = MQTT_STATETOPIC;
  doc["unit_of_meas"] = "%";
  doc["dev_cla"] = "humidity";
  doc["frc_upd"] = true;
  doc["uniq_id"] = String(MQTT_HOSTNAME) + "_humidity";
  doc["val_tpl"] = "{{value_json.humidity | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  mqttClient->publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTVoltageDiscoveryMsg(String MQTT_STATETOPIC, String MQTT_HOSTNAME) {
  String discoveryTopic =
      "homeassistant/sensor/" + String(MQTT_HOSTNAME) + "_voltage/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = "Garge " + CHIP_ID_STRING + " Voltage";
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = MQTT_STATETOPIC;
  doc["unit_of_meas"] = "V";
  doc["dev_cla"] = "voltage";
  doc["frc_upd"] = true;
  doc["uniq_id"] = String(MQTT_HOSTNAME) + "_voltage";
  doc["val_tpl"] = "{{value_json.voltage | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  mqttClient->publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTWizDiscoveryMsg(std::string deviceIP, std::string deviceName) {
  std::string discoveryTopic = "homeassistant/switch/" + deviceName + "/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = deviceName;
  doc["command_topic"] = "home/storage/" + deviceName + "/set";
  doc["state_topic"] = "home/storage/" + deviceName + "/state";
  doc["payload_on"] = "ON";
  doc["payload_off"] = "OFF";
  doc["optimistic"] = false;
  doc["qos"] = 1;
  doc["retain"] = true;
  doc["uniq_id"] = deviceName;
  doc["device"]["identifiers"] = deviceName;
  doc["device"]["name"] = deviceName;
  doc["device"]["model"] = "Wiz Light";
  doc["device"]["manufacturer"] = "Wiz";

  size_t n = serializeJson(doc, buffer);
  printHelper.print("Sending discovery for: ");
  printHelper.print(deviceName.c_str());
  printHelper.print(" discoveryTopic: ");
  printHelper.println(discoveryTopic.c_str());
  printHelper.print(" size: ");
  printHelper.println(String(n));
  printHelper.println("message: ");
  printHelper.println(buffer);

  mqttClient->publish(discoveryTopic.c_str(), buffer, n);
}

void publishWizState(String deviceName, bool lightState) {
  printHelper.println("publishWizState...");

  String payload = lightState ? "ON" : "OFF";
  String stateTopic = "home/storage/" + deviceName + "/state";
  mqttClient->publish(stateTopic.c_str(), payload.c_str());

  printHelper.println(deviceName);
  printHelper.println(payload);
  printHelper.println(stateTopic);
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  LOG("INFO", "Message arrived [%s]", topic);

  printHelper.print("Message arrived [");
  printHelper.print(topic);
  printHelper.println("] ");

  String payloadStr;
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += static_cast<char>(payload[i]);
  }
  printHelper.println(payloadStr);

  // Get the device MAC from the topic
  std::string topicStr(topic);
  size_t lastUnderscore = topicStr.find_last_of("_");
  size_t start = topicStr.find_last_of("_", lastUnderscore - 1) + 1;
  size_t lastSlash = topicStr.find_last_of("/");
  std::string deviceMac =
      topicStr.substr(lastUnderscore + 1, lastSlash - lastUnderscore - 1);
  std::string moduleName = topicStr.substr(start, lastUnderscore - start);
  printHelper.println(topicStr.c_str());

  // Find the device IP using the MAC address
  std::string deviceIP;
  for (const auto &device : liz::getDiscoveredDevices()) {
    if (std::get<1>(device) == deviceMac) {
      deviceIP = std::get<0>(device);
      break;
    }
  }

  printHelper.println(deviceIP.c_str());
  printHelper.println(String(port));
  printHelper.println(payloadStr);

  // If the payload is "on", turn on the light/switch
  if (payloadStr == "ON") {
    liz::setPilot(deviceIP.c_str(), port, true);
  } else if (payloadStr == "OFF") {
    liz::setPilot(deviceIP.c_str(), port, false);
  }

  auto response = liz::getPilot(deviceIP.c_str(), port);
  if (response) {
    printHelper.println(response->c_str());
    printHelper.println("");

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response->c_str());
    if (error) {
      printHelper.println("Failed to parse JSON response");
    } else {
      bool state = doc["result"]["state"];

      // Filter out "SOCKET" or "SHRGBC"
      std::smatch match;
      if (std::regex_search(moduleName, match, std::regex("(SOCKET|SHRGBC)"))) {
        moduleName = match.str();
      }

      std::string deviceName = "wiz_" + moduleName + "_" + deviceMac;

      printHelper.print("MAC: ");
      printHelper.println(deviceMac.c_str());
      printHelper.print("State: ");
      printHelper.println(state ? "true" : "false");
      publishWizState(deviceName.c_str(), state);
    }
  }
}

bool mqttStatus() { return mqttClient->connected(); }

void connectToMQTT() {
  LOG("INFO", "Attempting to connect to MQTT broker: %s", MQTT_BROKER);
  printHelper.print("Attempting to connect to MQTT broker: ");
  printHelper.println(MQTT_BROKER);

  mqttClient->setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient->setBufferSize(512);
  mqttClient->setCallback(mqttCallback);

  String lastUsername = EEPROM_MQTT_USERNAME;
  String lastPassword = EEPROM_MQTT_PASSWORD;

  while (!mqttClient->connected()) {
    liz::clearDiscoveredDevices();
    printHelper.println("Clearing Discovered Devices");

    // Extra debug: Print WiFi and socket status
    LOG("DEBUG", "WiFi.status(): %d, IP: %s", WiFi.status(),
        WiFi.localIP().toString().c_str());
    LOG("DEBUG", "WiFi RSSI: %d", WiFi.RSSI());
    char errbuf[128];
    int errcode = secureClient->lastError(errbuf, sizeof(errbuf));
    LOG("DEBUG",
        "SecureClient connected(): %d, available(): %d, "
        "lastError(): %d, msg: %s",
        secureClient->connected(), secureClient->available(), errcode, errbuf);

    LOG("DEBUG", "Free heap: %u", ESP.getFreeHeap());

    if (EEPROM_MQTT_USERNAME != lastUsername ||
        EEPROM_MQTT_PASSWORD != lastPassword) {
      LOG("INFO",
          "Detected updated MQTT credentials, breaking out of connect loop.");
      break;
    }

    LOG("DEBUG", "Calling mqttClient->connect()...");
    if (mqttClient->connect(MQTT_HOSTNAME, EEPROM_MQTT_USERNAME.c_str(),
                            EEPROM_MQTT_PASSWORD.c_str())) {
      LOG("INFO", "MQTT connected");
      printHelper.println("MQTT connected");

      if (strcmp(GARGE_TYPE, "sensor") == 0) {
        sendMQTTTemperatureDiscoveryMsg(MQTT_STATETOPIC, MQTT_HOSTNAME);
        sendMQTTHumidityDiscoveryMsg(MQTT_STATETOPIC, MQTT_HOSTNAME);
      } else if (strcmp(GARGE_TYPE, "voltmeter") == 0) {
        sendMQTTVoltageDiscoveryMsg(MQTT_STATETOPIC, MQTT_HOSTNAME);
      }
    } else {
      LOG("ERROR", "MQTT connection failed! Error code = %d",
          mqttClient->state());
      printHelper.print("MQTT connection failed! Error code = ");
      printHelper.println(String(mqttClient->state()));

      errcode = secureClient->lastError(errbuf, sizeof(errbuf));
      LOG("DEBUG", "SecureClient connected(): %d, lastError(): %d, msg: %s",
          secureClient->connected(), errcode, errbuf);
      LOG("DEBUG", "WiFi.status(): %d, IP: %s", WiFi.status(),
          WiFi.localIP().toString().c_str());
    }

    int64_t waitStart = millis();
    while (millis() - waitStart < 5000) {
      checkSerialForCredentials();
      delay(10);
    }
  }
}
