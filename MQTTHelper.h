#ifndef MQTTHELPER_H
#define MQTTHELPER_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "PRINTHelper.h"

extern String MQTT_STATETOPIC;
extern const char* MQTT_BROKER;
extern const char* MQTT_HOSTNAME;
extern const char* MQTT_PASS;
extern const char* MQTT_USER;
extern const int MQTT_PORT;

extern std::vector<std::pair<String, String>> discoveredDevices;
extern PRINTHelper printHelper;
WiFiClient espClient;
PubSubClient client(espClient);

void sendMQTTTemperatureDiscoveryMsg(String MQTT_STATETOPIC, String MQTT_HOSTNAME) {
  String discoveryTopic = "homeassistant/sensor/" + String(MQTT_HOSTNAME) + "_temperature/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = String(MQTT_HOSTNAME) + "_temperature";
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = MQTT_STATETOPIC;
  doc["unit_of_meas"] = "°C";
  doc["dev_cla"] = "temperature";
  doc["frc_upd"] = true;
  doc["uniq_id"] = String(MQTT_HOSTNAME) + "_temperature";
  doc["val_tpl"] = "{{value_json.temperature | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  client.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTHumidityDiscoveryMsg(String MQTT_STATETOPIC, String MQTT_HOSTNAME) {
  String discoveryTopic = "homeassistant/sensor/" + String(MQTT_HOSTNAME) + "_humidity/config";

  DynamicJsonDocument doc(1024);
  char buffer[512];

  doc["name"] = String(MQTT_HOSTNAME) + "_humidity";
  doc["stat_cla"] = "measurement";
  doc["stat_t"] = MQTT_STATETOPIC;
  doc["unit_of_meas"] = "%";
  doc["dev_cla"] = "humidity";
  doc["frc_upd"] = true;
  doc["uniq_id"] = String(MQTT_HOSTNAME) + "_humidity";
  doc["val_tpl"] = "{{value_json.humidity | round(3) | default(0)}}";

  size_t n = serializeJson(doc, buffer);

  client.publish(discoveryTopic.c_str(), buffer, n);
}

void sendMQTTWizDiscoveryMsg(String deviceIP, String deviceName) {
  String discoveryTopic = "homeassistant/switch/" + deviceName + "/config";

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

  client.publish(discoveryTopic.c_str(), buffer, n);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String payloadStr;
  for (unsigned int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  printHelper.println(payloadStr);

  // TODO: Add code here to send a command to the Wiz light based on the payload
}

void publishWizState(String deviceName, bool lightState) {
  String payload = lightState ? "ON" : "OFF";

  String stateTopic = "home/storage/" + deviceName + "/state";
  client.publish(stateTopic.c_str(), payload.c_str());
}

void connectToMQTT() {
  Serial.print("Attempting to connect to MQTT broker: ");
  Serial.print(MQTT_BROKER);

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setBufferSize(512);
  client.setCallback(mqttCallback);

  while (!client.connected()) {
    Serial.print(".");

    if (client.connect(MQTT_HOSTNAME, MQTT_USER, MQTT_PASS)) {
      Serial.println("");
      Serial.println("MQTT connected");

      sendMQTTTemperatureDiscoveryMsg(MQTT_STATETOPIC, MQTT_HOSTNAME);
      sendMQTTHumidityDiscoveryMsg(MQTT_STATETOPIC, MQTT_HOSTNAME);
    } else {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(client.state());
    }
  }
}

#endif