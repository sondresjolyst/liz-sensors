#ifndef SENSORCONTROLLER_H
#define SENSORCONTROLLER_H

#include <DHT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <cmath>

#include "PRINTHelper.h"
#include "PRINTHelper.h"

extern DHT dht;
extern Adafruit_BME280 bme;
extern WiFiClient serverClient;
extern PubSubClient client;
extern String MQTT_STATETOPIC;

float BMEtempOffset = -1;
float BMEhumidOffset = 9;

float DHTtempOffset = -3;
float DHThumidOffset = 6;

const int READ_DELAY = 60000;
const int READING_BUFFER = 5;
float averageHumid = 0;
float averageTemp = 0;
float humidReadings[READING_BUFFER];
float tempReadings[READING_BUFFER];
float totalHumid = 0;
float totalTemp = 0;
int readIndex = 0;

float currentTempReadings = 0;
float currentHumidReadings = 0;
int failedTempReadings = 0;
int failedHumidReadings = 0;

extern PRINTHelper printHelper;

void environmentalSensorSetup(const char *SENSOR_TYPE)
{
  if (strcmp(SENSOR_TYPE, "DHT") == 0)
  {
    Serial.printf("Sensor type is: %s\n", SENSOR_TYPE);
    dht.begin();

    for (int i = 0; i < READING_BUFFER; i++)
    {
      tempReadings[i] = dht.readTemperature() + DHTtempOffset;
      humidReadings[i] = dht.readHumidity() + DHThumidOffset;
      totalTemp += tempReadings[i];
      totalHumid += humidReadings[i];
    }
  }
  else if (strcmp(SENSOR_TYPE, "BME") == 0)
  {
    Serial.printf("Sensor type is: %s\n", SENSOR_TYPE);
    Wire.begin();
    if (!bme.begin(0x76))
    {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1)
        ;
    }
    for (int i = 0; i < READING_BUFFER; i++)
    {
      tempReadings[i] = bme.readTemperature() + BMEtempOffset;
      humidReadings[i] = bme.readHumidity() + BMEhumidOffset;
      totalTemp += tempReadings[i];
      totalHumid += humidReadings[i];
    }
  }
  else
  {
    Serial.print("Error: No sensor type selected!");
  }
}

void checkAndRestartIfFailed(float *reading, int &failedReadings)
{
  printHelper.println("Checking if reading failed");
  if (reading == nullptr || std::isnan(*reading))
  {
    printHelper.printf("Reading: %s", String(*reading));
    failedReadings += 1;
    printHelper.printf("Failed count: %s", String(failedReadings));
    if (failedReadings >= 10)
    {
      ESP.restart();
    }
  }
  else
  {
    printHelper.println("Reading OK");
    printHelper.printf("Reading: %s", String(*reading));
    failedReadings = 0;
  }
}

void readAndWriteEnvironmentalSensors(const char *SENSOR_TYPE)
{
  static unsigned long lastToggleTime = 0;

  if (millis() - lastToggleTime >= READ_DELAY)
  {
    int arrayLength = sizeof(tempReadings) / sizeof(tempReadings[0]);

    lastToggleTime = millis();

    totalTemp -= tempReadings[readIndex];
    totalHumid -= humidReadings[readIndex];

    if (strcmp(SENSOR_TYPE, "DHT") == 0)
    {
      currentTempReadings = dht.readTemperature();
      currentHumidReadings = dht.readHumidity();

      tempReadings[readIndex] = currentTempReadings + DHTtempOffset;
      humidReadings[readIndex] = currentHumidReadings + DHThumidOffset;
    }
    if (strcmp(SENSOR_TYPE, "BME") == 0)
    {
      currentTempReadings = bme.readTemperature();
      currentHumidReadings = bme.readHumidity();

      tempReadings[readIndex] = currentTempReadings + BMEtempOffset;
      humidReadings[readIndex] = currentHumidReadings + BMEhumidOffset;
    }

    checkAndRestartIfFailed(&currentTempReadings, failedTempReadings);
    checkAndRestartIfFailed(&currentHumidReadings, failedHumidReadings);

    totalTemp += tempReadings[readIndex];
    totalHumid += humidReadings[readIndex];

    readIndex = (readIndex + 1) % arrayLength;

    averageTemp = totalTemp / arrayLength;
    averageHumid = totalHumid / arrayLength;

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