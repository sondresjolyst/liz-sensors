// Copyright (c) 2023-2025 Sondre Sjølyst

#include "SensorController.h"
#include "../helpers/MQTTHelper.h"

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 18
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 17
#endif

float BMEtempOffset = -3.49;
float BMEhumidOffset = 15;

float DHTtempOffset = -3;
float DHThumidOffset = 6;

float averageHumid = 0;
float averageTemp = 0;
float humidReadings[READING_BUFFER];
float tempReadings[READING_BUFFER];
float totalHumid = 0;
float totalTemp = 0;
int readIndex = 0;

float currentTempReadings = 0;
float currentHumidReadings = 0;
int32_t failedTempReadings = 0;
int32_t failedHumidReadings = 0;

void environmentalSensorSetup(const char *sensorType) {
  if (strcmp(sensorType, "dht") == 0) {
    printHelper.log("INFO", "Sensor type is: %s", sensorType);
    dht.begin();

    for (int i = 0; i < READING_BUFFER; i++) {
      tempReadings[i] = dht.readTemperature() + DHTtempOffset;
      humidReadings[i] = dht.readHumidity() + DHThumidOffset;
      totalTemp += tempReadings[i];
      totalHumid += humidReadings[i];
    }
  } else if (strcmp(sensorType, "bme") == 0) {
    printHelper.log("INFO", "Sensor type is: %s", sensorType);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    if (!bme.begin(0x76)) {
      printHelper.log("ERROR",
                      "Could not find a valid BME280 sensor, check wiring!");
      // while (1) {
      // }
    }
    for (int i = 0; i < READING_BUFFER; i++) {
      tempReadings[i] = bme.readTemperature() + BMEtempOffset;
      humidReadings[i] = bme.readHumidity() + BMEhumidOffset;
      totalTemp += tempReadings[i];
      totalHumid += humidReadings[i];
    }
  } else {
    printHelper.log("ERROR", "No sensor type selected!");
  }
}

void checkAndRestartIfFailed(float *reading, int32_t *failedReadings) {
  printHelper.log("INFO", "Checking if reading failed");
  if (reading == nullptr || std::isnan(*reading)) {
    (*failedReadings) += 1;
    printHelper.log("ERROR", "Reading: %s, Failed count: %s", String(*reading),
                    String(*failedReadings));
    if (*failedReadings >= 10) {
      ESP.restart();
    }
  } else {
    printHelper.log("INFO", "Reading OK");
    printHelper.log("INFO", "Reading: %s", String(*reading));
    *failedReadings = 0;
  }
}

void readAndWriteEnvironmentalSensors(const char *sensorType) {
  static uint32_t lastToggleTime = 0;

  if (millis() - lastToggleTime >= READ_DELAY) {
    int arrayLength = sizeof(tempReadings) / sizeof(tempReadings[0]);

    lastToggleTime = millis();

    totalTemp -= tempReadings[readIndex];
    totalHumid -= humidReadings[readIndex];

    if (strcmp(sensorType, "dht") == 0) {
      currentTempReadings = dht.readTemperature();
      currentHumidReadings = dht.readHumidity();

      tempReadings[readIndex] = currentTempReadings + DHTtempOffset;
      humidReadings[readIndex] = currentHumidReadings + DHThumidOffset;
    }
    if (strcmp(sensorType, "bme") == 0) {
      currentTempReadings = bme.readTemperature();
      currentHumidReadings = bme.readHumidity();

      tempReadings[readIndex] = currentTempReadings + BMEtempOffset;
      humidReadings[readIndex] = currentHumidReadings + BMEhumidOffset;
    }

    totalTemp += tempReadings[readIndex];
    totalHumid += humidReadings[readIndex];

    checkAndRestartIfFailed(&totalTemp, &failedTempReadings);
    checkAndRestartIfFailed(&currentHumidReadings, &failedHumidReadings);

    printHelper.log("INFO", "tempReadings; %.2f °C, humidReadings: %.2f %%",
        tempReadings[readIndex],
        humidReadings[readIndex]);
    printHelper.log("INFO", "totalTemp: %.2f °C, totalHumid: %.2f %%",
        totalTemp, totalHumid);

    readIndex = (readIndex + 1) % arrayLength;

    printHelper.log("INFO", "readIndex: %d, arrayLength: %d", readIndex,
                    arrayLength);

    averageTemp = totalTemp / arrayLength;
    averageHumid = totalHumid / arrayLength;

    DynamicJsonDocument tempDoc(256);
    char tempBuffer[128];
    tempDoc["value"] = averageTemp;
    size_t tempN = serializeJson(tempDoc, tempBuffer);
    publishGargeSensorState(CHIP_ID, "temperature", String(tempBuffer));

    DynamicJsonDocument humidDoc(256);
    char humidBuffer[128];
    humidDoc["value"] = averageHumid;
    size_t humidN = serializeJson(humidDoc, humidBuffer);
    publishGargeSensorState(CHIP_ID, "humidity", String(humidBuffer));

    printHelper.log("INFO", "Temperature: %.2f °C, Humidity: %.2f %%",
                    averageTemp, averageHumid);
  }
}
