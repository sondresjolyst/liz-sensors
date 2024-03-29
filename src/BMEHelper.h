#ifndef BMEHELPER_H
#define BMEHELPER_H

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "PRINTHelper.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

extern Adafruit_BME280 bme;
extern WiFiClient serverClient;
extern PubSubClient client;

const int BME_NUM_READINGS = 5;
const int BME_READ_DELAY = 60000;
float BMEaverageTemp = 0;
float BMEaverageHumid = 0;
float BMEhumidReadings[BME_NUM_READINGS];
float BMEtempReadings[BME_NUM_READINGS];
float BMEtotalHumid = 0;
float BMEtotalTemp = 0;
float BMEtempOffset = -1;
float BMEhumidOffset = 9;
int BMEreadIndex = 0;
extern String MQTT_STATETOPIC;

extern PRINTHelper printHelper;

void readAndWriteBME()
{
    static unsigned long lastToggleTime = 0;
    if (millis() - lastToggleTime >= BME_READ_DELAY)
    {
        int arrayLength = sizeof(BMEtempReadings) / sizeof(BMEtempReadings[0]);

        lastToggleTime = millis();

        BMEtotalTemp -= BMEtempReadings[BMEreadIndex];
        BMEtotalHumid -= BMEhumidReadings[BMEreadIndex];

        BMEtempReadings[BMEreadIndex] = bme.readTemperature() + BMEtempOffset;
        BMEhumidReadings[BMEreadIndex] = bme.readHumidity() + BMEhumidOffset;

        BMEtotalTemp += BMEtempReadings[BMEreadIndex];
        BMEtotalHumid += BMEhumidReadings[BMEreadIndex];

        BMEreadIndex = (BMEreadIndex + 1) % arrayLength;

        BMEaverageTemp = BMEtotalTemp / arrayLength;
        BMEaverageHumid = BMEtotalHumid / arrayLength;

        DynamicJsonDocument doc(1024);
        char buffer[256];

        doc["temperature"] = BMEaverageTemp;
        doc["humidity"] = BMEaverageHumid;

        size_t n = serializeJson(doc, buffer);

        bool published = client.publish(MQTT_STATETOPIC.c_str(), buffer, n);

        Serial.println("Published: ");
        Serial.println(published);

        Serial.print("Temperature: ");
        Serial.print(BMEaverageTemp);
        Serial.println(" °C");

        Serial.print("Humidity: ");
        Serial.print(BMEaverageHumid);
        Serial.println(" %");
    }
}

#endif