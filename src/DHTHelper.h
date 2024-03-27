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

const int DHT_NUM_READINGS = 5;
const int DHT_READ_DELAY = 60000;
float DHTaverageHumid = 0;
float DHTaverageTemp = 0;
float DHThumidReadings[DHT_NUM_READINGS];
float DHTtempReadings[DHT_NUM_READINGS];
float DHTtotalHumid = 0;
float DHTtotalTemp = 0;
float DHTtempOffset = -3;
float DHThumidOffset = 6;
int DHTreadIndex = 0;
extern String MQTT_STATETOPIC;

extern PRINTHelper printHelper;

void readAndWriteDHT()
{
  static unsigned long lastToggleTime = 0;

  if (millis() - lastToggleTime >= DHT_READ_DELAY)
  {
    int arrayLength = sizeof(DHTtempReadings) / sizeof(DHTtempReadings[0]);

    lastToggleTime = millis();

    DHTtotalTemp -= DHTtempReadings[DHTreadIndex];
    DHTtotalHumid -= DHThumidReadings[DHTreadIndex];

    DHTtempReadings[DHTreadIndex] = dht.readTemperature() + DHTtempOffset;
    DHThumidReadings[DHTreadIndex] = dht.readHumidity() + DHThumidOffset;

    DHTtotalTemp += DHTtempReadings[DHTreadIndex];
    DHTtotalHumid += DHThumidReadings[DHTreadIndex];

    DHTreadIndex = (DHTreadIndex + 1) % arrayLength;

    DHTaverageTemp = DHTtotalTemp / arrayLength;
    DHTaverageHumid = DHTtotalHumid / arrayLength;

    DynamicJsonDocument doc(1024);
    char buffer[256];

    doc["temperature"] = DHTaverageTemp;
    doc["humidity"] = DHTaverageHumid;

    size_t n = serializeJson(doc, buffer);

    bool published = client.publish(MQTT_STATETOPIC.c_str(), buffer, n);

    Serial.println("Published: ");
    Serial.println(published);

    Serial.print("Temperature: ");
    Serial.print(DHTaverageTemp);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(DHTaverageHumid);
    Serial.println(" %");
  }
}

#endif