// Copyright (c) 2023-2025 Sondre Sjølyst

#include "VoltmeterController.h"
#include <esp_sleep.h>

RTC_DATA_ATTR float averageVoltage = 0;
RTC_DATA_ATTR float voltageReadings[READING_VOLTAGE_BUFFER];
RTC_DATA_ATTR float totalVoltage = 0;
RTC_DATA_ATTR int readVoltageIndex = 0;
RTC_DATA_ATTR bool bufferFilled = false;

float currentVoltageReadings = 0;
RTC_DATA_ATTR int32_t failedVoltageReadings = 0;

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
  printHelper.println("Checking if reading failed");
  if (reading == nullptr || std::isnan(*reading)) {
    printHelper.printf("Reading: %s", String(*reading));
    (*failedReadings) += 1;
    printHelper.printf("Failed count: %s", String(*failedReadings));
    if (*failedReadings >= 10) {
      ESP.restart();
    }
  } else {
    printHelper.println("Reading OK");
    printHelper.printf("Reading: %s", String(*reading));
    *failedReadings = 0;
  }
}

void deepSleepForHour() {
  printHelper.println("Entering deep sleep for 1 hour");
  Serial.println("Entering deep sleep for 1 hour");
  esp_sleep_enable_timer_wakeup(
      VOLTMETER_SLEEP_INTERVAL_US);
  Serial.flush();
  esp_deep_sleep_start();
}

void readAndWriteVoltageSensor() {
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

  // Go to deep sleep for 1 hour after publishing
  deepSleepForHour();
}
