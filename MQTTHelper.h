#ifndef MQTTHELPER_H
#define MQTTHELPER_H

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

extern String MQTT_STATETOPIC;
extern const char* MQTT_BROKER;
extern const char* MQTT_HOSTNAME;
extern const char* MQTT_PASS;
extern const char* MQTT_USER;
extern const int MQTT_PORT;

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
  doc["command_topic"] = "home/" + deviceName + "/set";
  doc["state_topic"] = "home/" + deviceName + "/state";
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

void connectToMQTT() {
  Serial.print("Attempting to connect to MQTT broker: ");
  Serial.print(MQTT_BROKER);

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setBufferSize(512);

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