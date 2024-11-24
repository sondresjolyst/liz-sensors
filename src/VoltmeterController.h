#ifndef VOLTMETERCONTROLLER_H
#define VOLTMETERCONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "PRINTHelper.h"

extern PubSubClient client;
extern String MQTT_STATETOPIC;
extern PRINTHelper printHelper;

const int ANALOG_IN_PIN = A0;
const int ANALOG_RESOLUTION = 1024;        // 10-bit resolution
const float ANALOG_VOLTAGE = 3.3;          // Reference voltage for ESP8266 ADC
const float R1 = 10000.0;                  // 10kΩ
const float R2 = 4700.0;                   // 4.7kΩ
const float ACTUAL_BATTERY_VOLTAGE = 9.65; // Measured voltage from multimeter
const float MEASURED_VOLTAGE = 9.98;       // Voltage output from ESP8266
const float CALIBRATION_FACTOR = ACTUAL_BATTERY_VOLTAGE / MEASURED_VOLTAGE;

const int READ_VOLTAGE_DELAY = 60000;
const int READING_VOLTAGE_BUFFER = 5;
float averageVoltage = 0;
float voltageReadings[READING_VOLTAGE_BUFFER];
float totalVoltage = 0;
int readVoltageIndex = 0;

float currentVoltageReadings = 0;
int failedVoltageReadings = 0;

float readVoltage()
{
  int sensorValue = analogRead(ANALOG_IN_PIN);
  Serial.println(sensorValue);

  // Calculate the voltage at the A0 pin
  float vout = (static_cast<float>(sensorValue) / ANALOG_RESOLUTION) * ANALOG_VOLTAGE;

  // Calculate the actual input voltage using the voltage divider formula
  float vin = vout * ((R1 + R2) / R2);

  // Apply the calibration factor
  vin *= CALIBRATION_FACTOR;

  // Suppress erroneous readings if needed
  if (vin < 0.09)
  {
    vin = 0.0;
  }

  return vin;
}

void voltageSensorSetup()
{
  pinMode(ANALOG_IN_PIN, INPUT);

  for (int i = 0; i < READING_VOLTAGE_BUFFER; i++)
  {
    voltageReadings[i] = readVoltage();
    totalVoltage += voltageReadings[i];
  }
}

void voltageCheckAndRestartIfFailed(float *reading, int &failedReadings)
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

void readAndWriteVoltageSensor()
{
  static unsigned long lastToggleTime = 0;

  if (millis() - lastToggleTime >= READ_VOLTAGE_DELAY)
  {
    int arrayLength = sizeof(voltageReadings) / sizeof(voltageReadings[0]);

    lastToggleTime = millis();

    totalVoltage -= voltageReadings[readVoltageIndex];
    voltageReadings[readVoltageIndex] = readVoltage();
    totalVoltage += voltageReadings[readVoltageIndex];

    voltageCheckAndRestartIfFailed(&totalVoltage, failedVoltageReadings);

    readVoltageIndex = (readVoltageIndex + 1) % arrayLength;
    averageVoltage = totalVoltage / arrayLength;

    DynamicJsonDocument doc(1024);
    char buffer[256];

    doc["voltage"] = averageVoltage;

    size_t n = serializeJson(doc, buffer);

    bool published = client.publish(MQTT_STATETOPIC.c_str(), buffer, n);

    Serial.println("Published: ");
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

#endif