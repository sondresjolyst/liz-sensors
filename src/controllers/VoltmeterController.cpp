// Copyright (c) 2023-2025 Sondre Sjølyst

#include <esp_sleep.h>

#include "../helpers/MQTTHelper.h"
#include "VoltmeterController.h"

RTC_DATA_ATTR float averageVoltage = 0;
RTC_DATA_ATTR float voltageReadings[READING_VOLTAGE_BUFFER];
RTC_DATA_ATTR float totalVoltage = 0;
RTC_DATA_ATTR int readVoltageIndex = 0;
RTC_DATA_ATTR bool bufferFilled = false;

float currentVoltageReadings = 0;
RTC_DATA_ATTR int32_t failedVoltageReadings = 0;

float readVoltage() {
  int sensorValue = analogRead(ANALOG_IN_PIN);
  printHelper.log("INFO", "Sensor Value value: %d", sensorValue);

  // Calculate the measured voltage at the divider output
  float voltageMeasured = (ANALOG_VOLTAGE / ANALOG_RESOLUTION) * sensorValue;
  printHelper.log("INFO", "voltageMeasured: %.5f V", voltageMeasured);

  float vinTest = voltageMeasured * ((R1 + R2) / R2);
  printHelper.log("INFO", "vinTest: %.5f V", vinTest);

  // calculated correction
  float vinMeasuredCorrected =
      voltageMeasured * ((R1 + R2) / R2 * CORRECTION_FACTOR);
  printHelper.log("INFO", "vinMeasuredCorrected: %.5f V", vinMeasuredCorrected);

  // exponential correction
  float vinTestCorrectedExponential = a * pow(vinTest, b);
  printHelper.log("INFO", "vinTestCorrectedExponential: %.5f V",
                  vinTestCorrectedExponential);

  return vinTestCorrectedExponential;
}

void voltageSensorSetup() {
  pinMode(ANALOG_IN_PIN, INPUT);

  if (!bufferFilled) {
    totalVoltage = 0;
    for (int i = 0; i < READING_VOLTAGE_BUFFER; i++) {
      voltageReadings[i] = readVoltage();
      totalVoltage += voltageReadings[i];
    }
    bufferFilled = true;
  }
}

void voltageCheckAndRestartIfFailed(float *reading, int32_t *failedReadings) {
  printHelper.log("INFO", "Checking if reading failed");
  if (reading == nullptr || std::isnan(*reading)) {
    printHelper.log("ERROR", "Reading: %s", String(*reading));
    (*failedReadings) += 1;
    printHelper.log("ERROR", "Failed count: %s", String(*failedReadings));
    if (*failedReadings >= 10) {
      ESP.restart();
    }
  } else {
    printHelper.log("INFO", "Reading OK");
    printHelper.log("INFO", "Reading: %s", String(*reading));
    *failedReadings = 0;
  }
}

void deepSleepForHour() {
  printHelper.log("INFO", "Entering deep sleep for 1 hour");
  esp_sleep_enable_timer_wakeup(VOLTMETER_SLEEP_INTERVAL_US);
  Serial.flush();
  esp_deep_sleep_start();
}

void readAndWriteVoltageSensor() {
  static uint32_t lastVoltageAttempt = 0;
  const uint32_t voltageReadInterval = 5000;  // 5 seconds

  if (millis() - lastVoltageAttempt < voltageReadInterval) {
    return;
  }
  lastVoltageAttempt = millis();

  if (!bufferFilled) {
    voltageSensorSetup();
  }
  int arrayLength = sizeof(voltageReadings) / sizeof(voltageReadings[0]);

  totalVoltage -= voltageReadings[readVoltageIndex];
  voltageReadings[readVoltageIndex] = readVoltage();
  totalVoltage += voltageReadings[readVoltageIndex];

  voltageCheckAndRestartIfFailed(&totalVoltage, &failedVoltageReadings);

  readVoltageIndex = (readVoltageIndex + 1) % arrayLength;
  averageVoltage = totalVoltage / arrayLength;

  DynamicJsonDocument doc(1024);
  char buffer[256];

  doc["value"] = averageVoltage;
  size_t n = serializeJson(doc, buffer);

  bool publishSuccess =
      publishGargeSensorState(CHIP_ID, "voltage", String(buffer));

  printHelper.log("INFO", "Battery Voltage: %.5f V", averageVoltage);

  if (!mqttStatus()) {
    printHelper.log("WARN", "MQTT not connected, skipping voltage publish.");
    return;
  }

  if (publishSuccess) {
    deepSleepForHour();
  } else {
    printHelper.log("WARN", "Publish failed, not entering deep sleep.");
  }
}
