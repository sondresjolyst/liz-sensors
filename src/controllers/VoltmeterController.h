// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_CONTROLLERS_VOLTMETERCONTROLLER_H_
#define SRC_CONTROLLERS_VOLTMETERCONTROLLER_H_

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include "helpers/PRINTHelper.h"

extern PubSubClient *mqttClient;
extern String CHIP_ID_STRING;
extern PRINTHelper printHelper;

const int ANALOG_IN_PIN = A0;
const int ANALOG_RESOLUTION = 4096;     // 10-bit resolution
const float ANALOG_VOLTAGE = 3.32;      // Reference voltage for ESP32 ADC
const float R1 = 33000.0;               // 47kΩ
const float R2 = 10000.0;               // 4.7kΩ
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

// Deep sleep interval for voltmeter (in microseconds)
constexpr uint64_t VOLTMETER_SLEEP_INTERVAL_US =
    3600ULL * 1000000ULL;  // 1 hour

extern float averageVoltage;
extern float voltageReadings[READING_VOLTAGE_BUFFER];
extern float totalVoltage;
extern int readVoltageIndex;

extern float currentVoltageReadings;
extern int32_t failedVoltageReadings;

float readVoltage();
void voltageSensorSetup();
void voltageCheckAndRestartIfFailed(float *reading, int32_t *failedReadings);
void readAndWriteVoltageSensor();

#endif  // SRC_CONTROLLERS_VOLTMETERCONTROLLER_H_
