#ifndef DHTHELPER_H
#define DHTHELPER_H

#include <DHT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "PRINTHelper.h"

extern DHT dht;
extern WiFiClient serverClient;
// extern WiFiClient espClient;
extern PubSubClient client;

const int DHT_NUM_READINGS = 10;
const int DHT_READ_DELAY = 2000;
float averageHumid = 0;
float averageTemp = 0;
float humidReadings[DHT_NUM_READINGS];
float tempReadings[DHT_NUM_READINGS];
float totalHumid = 0;
float totalTemp = 0;
float tempOffet = -3;
float humidOffset = 6;
int readIndex = 0;
extern String MQTT_STATETOPIC;

PRINTHelper printHelper(serverClient);

void readAndWriteDHT()
{
  static unsigned long lastToggleTime = 0;

  if (millis() - lastToggleTime >= DHT_READ_DELAY)
  {
    lastToggleTime = millis();

    totalTemp -= tempReadings[readIndex];
    totalHumid -= humidReadings[readIndex];

    tempReadings[readIndex] = dht.readTemperature() + tempOffet;
    humidReadings[readIndex] = dht.readHumidity() + humidOffset;

    totalTemp += tempReadings[readIndex];
    totalHumid += humidReadings[readIndex];

    readIndex = (readIndex + 1) % DHT_NUM_READINGS;

    averageTemp = totalTemp / DHT_NUM_READINGS;
    averageHumid = totalHumid / DHT_NUM_READINGS;

    DynamicJsonDocument doc(1024);
    char buffer[256];

    doc["temperature"] = averageTemp;
    doc["humidity"] = averageHumid;

    size_t n = serializeJson(doc, buffer);

    bool published = client.publish(MQTT_STATETOPIC.c_str(), buffer, n);

    Serial.println("Published: ");
    Serial.println(published);

    Serial.print("Temperature: ");
    Serial.print(averageTemp);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(averageHumid);
    Serial.println(" %");
  }
}

#endif