// Copyright (c) 2023-2025 Sondre Sjølyst

#include <string>

#include "MQTTHelper.h"

extern String MQTT_HOSTNAME_STRING;

WiFiClient espClient;
PubSubClient client(espClient);

void sendMQTTTemperatureDiscoveryMsg(const String &discoveredDeviceId,
                                     const String &MQTT_STATETOPIC) {
  String discoveryTopic = "garge/devices/sensors/" + MQTT_HOSTNAME_STRING +
                          "/" + discoveredDeviceId + "/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = "Garge " + CHIP_ID_STRING + " Temperature";
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = MQTT_STATETOPIC;
  doc["unit_of_meas"] = "°C";
  doc["dev_cla"] = "temperature";
  doc["frc_upd"] = true;
  doc["uniq_id"] = discoveredDeviceId + "_temperature";
  doc["val_tpl"] = "{{value_json.temperature | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  client.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTHumidityDiscoveryMsg(const String &discoveredDeviceId,
                                  const String &MQTT_STATETOPIC) {
  String discoveryTopic = "garge/devices/sensors/" + MQTT_HOSTNAME_STRING +
                          "/" + discoveredDeviceId + "/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = "Garge " + CHIP_ID_STRING + " Humidity";
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = MQTT_STATETOPIC;
  doc["unit_of_meas"] = "%";
  doc["dev_cla"] = "humidity";
  doc["frc_upd"] = true;
  doc["uniq_id"] = discoveredDeviceId + "_humidity";
  doc["val_tpl"] = "{{value_json.humidity | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  client.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTVoltageDiscoveryMsg(const String &discoveredDeviceId,
                                 const String &MQTT_STATETOPIC) {
  String discoveryTopic = "garge/devices/sensors/" + MQTT_HOSTNAME_STRING +
                          "/" + discoveredDeviceId + "/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = "Garge " + CHIP_ID_STRING + " Voltage";
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = MQTT_STATETOPIC;
  doc["unit_of_meas"] = "V";
  doc["dev_cla"] = "voltage";
  doc["frc_upd"] = true;
  doc["uniq_id"] = discoveredDeviceId + "_voltage";
  doc["val_tpl"] = "{{value_json.voltage | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  client.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTSocketDiscoveryMsg(const std::string &deviceIP,
                                const std::string &deviceName,
                                const std::string &model,
                                const std::string &manufacturer) {
  std::string sensorId = MQTT_HOSTNAME_STRING.c_str();
  std::string discoveryTopic =
      "garge/devices/sockets/" + sensorId + "/" + deviceName + "/config";

  DynamicJsonDocument doc(1024);
  char buffer[1024];

  doc["name"] = deviceName;
  doc["command_topic"] =
      "garge/devices/sockets/" + sensorId + "/" + deviceName + "/set";
  doc["state_topic"] =
      "garge/devices/sockets/" + sensorId + "/" + deviceName + "/state";
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

  size_t n = serializeJson(doc, buffer);
  printHelper.print("Sending discovery for: ");
  printHelper.print(deviceName.c_str());
  printHelper.print(" discoveryTopic: ");
  printHelper.println(discoveryTopic.c_str());
  printHelper.print(" size: ");
  printHelper.println(String(n));
  printHelper.println("message: ");
  printHelper.println(buffer);

  bool pubResult = false;
  if (client.connected()) {
    pubResult = client.publish(discoveryTopic.c_str(), (const uint8_t *)buffer,
                               n, false);
    printHelper.print("Publish result (retain=false): ");
    printHelper.println(pubResult ? "success" : "fail");
    printHelper.print("MQTT client state: ");
    printHelper.println(String(client.state()));
  } else {
    printHelper.println("Publish skipped: MQTT not connected");
  }
}

void publishSocketState(const String &deviceName, bool socketState) {
  printHelper.println("publishSocketState...");

  String payload = socketState ? "ON" : "OFF";
  String stateTopic = "garge/devices/sockets/" + MQTT_HOSTNAME_STRING + "/" +
                      deviceName + "/state";
  client.publish(stateTopic.c_str(), payload.c_str());

  printHelper.println(deviceName);
  printHelper.println(payload);
  printHelper.println(stateTopic);
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

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

  // If the payload is "on", turn on the socket
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

      std::string deviceName = "socket_" + moduleName + "_" + deviceMac;

      printHelper.print("MAC: ");
      printHelper.println(deviceMac.c_str());
      printHelper.print("State: ");
      printHelper.println(state ? "true" : "false");
      publishSocketState(deviceName.c_str(), state);
    }
  }
}

bool mqttStatus() { return client.connected(); }

void connectToMQTT() {
  Serial.print("Attempting to connect to MQTT broker: ");
  Serial.println(MQTT_BROKER);
  printHelper.print("Attempting to connect to MQTT broker: ");
  printHelper.println(MQTT_BROKER);

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setBufferSize(1024);
  client.setCallback(mqttCallback);

  while (!client.connected()) {
    liz::clearDiscoveredDevices();
    printHelper.println("Clearing Discovered Devices");
    if (client.connect(MQTT_HOSTNAME, MQTT_USER, MQTT_PASS)) {
      Serial.println("");
      Serial.println("MQTT connected");
      printHelper.println("");
      printHelper.println("MQTT connected");

      if (strcmp(GARGE_TYPE, "sensor") == 0) {
        sendMQTTTemperatureDiscoveryMsg(MQTT_HOSTNAME_STRING + "_temperature",
                                        MQTT_STATETOPIC);
        sendMQTTHumidityDiscoveryMsg(MQTT_HOSTNAME_STRING + "_humidity",
                                     MQTT_STATETOPIC);
      } else if (strcmp(GARGE_TYPE, "voltmeter") == 0) {
        sendMQTTVoltageDiscoveryMsg(MQTT_HOSTNAME_STRING, MQTT_STATETOPIC);
        sendMQTTVoltageDiscoveryMsg(MQTT_HOSTNAME_STRING + "_voltage",
                                    MQTT_STATETOPIC);
      }
    } else {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(client.state());
      printHelper.print("MQTT connection failed! Error code = ");
      printHelper.println(String(client.state()));
    }
  }
}
