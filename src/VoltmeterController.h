// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_VOLTMETERCONTROLLER_H_
#define SRC_VOLTMETERCONTROLLER_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "PRINTHelper.h"

extern PubSubClient client;
extern String MQTT_STATETOPIC;
extern PRINTHelper printHelper;

const int ANALOG_IN_PIN = A0;
const int ANALOG_RESOLUTION = 1024;     // 10-bit resolution
const float ANALOG_VOLTAGE = 3.32;      // Reference voltage for ESP8266 ADC
const float R1 = 47000.0;               // 47kΩ
const float R2 = 4700.0;                // 4.7kΩ
const float CORRECTION_FACTOR = 1.218;  // multimeter voltage / voltageMeasured

// Corrected constants for exponential correction
// Constants a and b are calculated based on curve fitting using known points:
// Point 1: (Measured Voltage: 5.31395, Actual Voltage: 5.03)
// Point 2: (Measured Voltage: 13.05305, Actual Voltage: 12.59)
// Steps to calculate a and b:
// 1. Convert the known points to natural logarithms.
// 2. Solve the system of linear equations to find b.
// 3. Use b to solve for a.
//
// Using points (5.31395, 5.03) and (13.05305, 12.59):
// ln(5.03) = ln(a) + b * ln(5.31395)
// ln(12.59) = ln(a) + b * ln(13.05305)
//
// Solve for b:
// b = (ln(12.59) - ln(5.03)) / (ln(13.05305) - ln(5.31395))
// b ≈ 1.02092
//
// Solve for a:
// a = exp(ln(5.03) - b * ln(5.31395))
// a ≈ 0.91406
const float a = 0.91406;
const float b = 1.02092;

const int READ_VOLTAGE_DELAY = 60000;
const int READING_VOLTAGE_BUFFER = 5;
float averageVoltage = 0;
float voltageReadings[READING_VOLTAGE_BUFFER];
float totalVoltage = 0;
int readVoltageIndex = 0;

float currentVoltageReadings = 0;
int failedVoltageReadings = 0;

float readVoltage() {
  int sensorValue = analogRead(ANALOG_IN_PIN);
  Serial.print("Sensor Value: ");
  Serial.println(sensorValue);

  printHelper.print("sensorValue: ");
  printHelper.print(String(sensorValue));
  printHelper.println(" V");

  // Calculate the measured voltage at the divider output
  float voltageMeasured = (ANALOG_VOLTAGE / ANALOG_RESOLUTION) * sensorValue;
  Serial.print("voltageMeasured: ");
  Serial.print(voltageMeasured, 5);
  Serial.println(" V");

  printHelper.print("voltageMeasured: ");
  printHelper.print(String(voltageMeasured, 5));
  printHelper.println(" V");

  float vinTest = voltageMeasured * ((R1 + R2) / R2);
  Serial.print("vinTest: ");
  Serial.print(vinTest, 5);
  Serial.println(" V");

  printHelper.print("vinTest: ");
  printHelper.print(String(vinTest, 5));
  printHelper.println(" V");

  // calculated correction
  float vinMeasuredCorrected =
      voltageMeasured * ((R1 + R2) / R2 * CORRECTION_FACTOR);
  Serial.print("vinMeasuredCorrected: ");
  Serial.print(vinMeasuredCorrected, 5);
  Serial.println(" V");

  printHelper.print("vinMeasuredCorrected: ");
  printHelper.print(String(vinMeasuredCorrected, 5));
  printHelper.println(" V");

  // exponential correction
  float vinTestCorrectedExponential = a * pow(vinTest, b);
  Serial.print("vinTestCorrectedExponential: ");
  Serial.print(vinTestCorrectedExponential, 5);
  Serial.println(" V");

  printHelper.print("vinTestCorrectedExponential: ");
  printHelper.print(String(vinTestCorrectedExponential, 5));
  printHelper.println(" V");

  return vinTestCorrectedExponential;
}

void voltageSensorSetup() {
  pinMode(ANALOG_IN_PIN, INPUT);

  for (int i = 0; i < READING_VOLTAGE_BUFFER; i++) {
    voltageReadings[i] = readVoltage();
    totalVoltage += voltageReadings[i];
  }
}

void voltageCheckAndRestartIfFailed(float *reading, int32_t *failedReadings) {
  printHelper.println("Checking if reading failed");
  if (reading == nullptr || std::isnan(*reading)) {
    printHelper.printf("Reading: %s", String(*reading));
    failedReadings += 1;
    printHelper.printf("Failed count: %s", String(*failedReadings));
    if (*failedReadings >= 10) {
      ESP.restart();
    }
  } else {
    printHelper.println("Reading OK");
    printHelper.printf("Reading: %s", String(*reading));
    failedReadings = 0;
  }
}

void readAndWriteVoltageSensor() {
  static uint32_t lastToggleTime = 0;

  if (millis() - lastToggleTime >= READ_VOLTAGE_DELAY) {
    int arrayLength = sizeof(voltageReadings) / sizeof(voltageReadings[0]);

    lastToggleTime = millis();

    totalVoltage -= voltageReadings[readVoltageIndex];
    voltageReadings[readVoltageIndex] = readVoltage();
    totalVoltage += voltageReadings[readVoltageIndex];

    voltageCheckAndRestartIfFailed(&totalVoltage, &failedVoltageReadings);

    readVoltageIndex = (readVoltageIndex + 1) % arrayLength;
    averageVoltage = totalVoltage / arrayLength;

    DynamicJsonDocument doc(1024);
    char buffer[256];

    doc["voltage"] = averageVoltage;

    size_t n = serializeJson(doc, buffer);

    bool published = client.publish(MQTT_STATETOPIC.c_str(), buffer, n);

    Serial.print("Published: ");
    Serial.println(published);
    // Debugging
    printHelper.print("Published: ");
    printHelper.println(String(published));

    Serial.print("Battery Voltage: ");
    Serial.print(averageVoltage);
    Serial.println(" V");
    // Debugging
    printHelper.print("Battery Voltage: ");
    printHelper.println(String(averageVoltage));
  }
}

#endif  // SRC_VOLTMETERCONTROLLER_H_
