// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_CONTROLLERS_SENSORCONTROLLER_H_
#define SRC_CONTROLLERS_SENSORCONTROLLER_H_

#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#include <cmath>
#include <cstdint>

#include "helpers/PRINTHelper.h"

extern DHT dht;
extern Adafruit_BME280 bme;
extern WiFiClientSecure *secureClient;
extern PubSubClient *mqttClient;
extern String CHIP_ID;
extern PRINTHelper printHelper;

extern float BMEtempOffset;
extern float BMEhumidOffset;
extern float DHTtempOffset;
extern float DHThumidOffset;

const int READ_DELAY = 60000;
const int READING_BUFFER = 5;

extern float averageHumid;
extern float averageTemp;
extern float humidReadings[READING_BUFFER];
extern float tempReadings[READING_BUFFER];
extern float totalHumid;
extern float totalTemp;
extern int readIndex;

extern float currentTempReadings;
extern float currentHumidReadings;
extern int32_t failedTempReadings;
extern int32_t failedHumidReadings;

void environmentalSensorSetup(const char *sensorType);
void checkAndRestartIfFailed(float *reading, int32_t *failedReadings);
void readAndWriteEnvironmentalSensors(const char *sensorType);

#endif  // SRC_CONTROLLERS_SENSORCONTROLLER_H_
