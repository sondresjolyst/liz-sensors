// Copyright (c) 2023-2025 Sondre Sjølyst

#include <Arduino.h>
#include <DHT.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

#include <cstdint>
#include <cstring>
#include <regex>
#include <string>

#include "./arduino_secrets.h"
#include "./liz.h"
#include "EEPROMHelper.h"
#include "MQTTHelper.h"
#include "OTAHelper.h"
#include "PRINTHelper.h"
#include "SensorController.h"
#include "VoltmeterController.h"
#include "WIZHelper.h"
#include "WebSite.h"
#include "WiFiHelper.h"

#ifndef SENSOR_TYPE
#define SENSOR_TYPE "BME"  // "BME" or "DHT"
#endif

#ifndef LIZ_TYPE
#define LIZ_TYPE "sensor"  // "voltmeter" or "sensor"
#endif

const char *MQTT_BROKER = SECRET_MQTTBROKER;
const char *MQTT_PASS = SECRET_MQTTPASS;
const char *MQTT_USER = SECRET_MQTTUSER;
const int MQTT_PORT = SECRET_MQTTPORT;

constexpr uint8_t DHTTYPE = DHT11;
constexpr float TEMP_HUMID_DIFF = 10.0;
constexpr int DHT_SENSOR_PIN = 2;
constexpr int DNS_PORT = 53;
constexpr int EEPROM_PASSWORD_END = 96;
constexpr int EEPROM_PASSWORD_START = 32;
constexpr int EEPROM_SSID_END = 31;
constexpr int EEPROM_SSID_START = 0;
constexpr int LED_BLINK_COUNT = 10;
constexpr int LED_BLINK_DELAY = 1000;
constexpr int RESET_BUTTON_GPO = 4;
constexpr int RESET_PRESS_DURATION = 5000;
constexpr int SERIAL_PORT = 9600;
constexpr int WEBSITE_PORT = 80;
constexpr int WIFI_DELAY = 1000;
constexpr int WIFI_TRIES = 15;
constexpr int port = 38899;
constexpr size_t EEPROM_SIZE = 512;

Adafruit_BME280 bme;
DHT dht(DHT_SENSOR_PIN, DHTTYPE, 11);
WebServer server(WEBSITE_PORT);
ResetWiFi resetWiFi(RESET_BUTTON_GPO, RESET_PRESS_DURATION);
OTAHelper *otaHelper = nullptr;
PRINTHelper printHelper(serverClient);

String CHIP_ID_STRING;
String MQTT_HOSTNAME_STRING;
const char *MQTT_HOSTNAME = nullptr;
String MQTT_STATETOPIC;
String WIFI_NAME;

void setup() {
  Serial.begin(SERIAL_PORT);
  delay(100);

  uint64_t chipId = ESP.getEfuseMac();
  CHIP_ID_STRING = String(chipId, HEX);
  MQTT_HOSTNAME_STRING = "Wemos_D1_Mini_" + CHIP_ID_STRING;
  MQTT_HOSTNAME = MQTT_HOSTNAME_STRING.c_str();
  MQTT_STATETOPIC = "home/storage/" + String(MQTT_HOSTNAME) + "/state";
  WIFI_NAME = "Liz Sensor " + String(MQTT_HOSTNAME);

  Serial.println("Disconnecting WiFi");
  WiFi.disconnect();

  EEPROM.begin(EEPROM_SIZE);
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Starting...");
  Serial.println("Reading EEPROM...");

  String EEPROM_SSID = readEEPROM(EEPROM_SSID_START, EEPROM_SSID_END);
  String EEPROM_PASSWORD =
      readEEPROM(EEPROM_PASSWORD_START, EEPROM_PASSWORD_END);

  if (EEPROM_SSID.isEmpty() || EEPROM_SSID.length() < 2 ||
      std::all_of(EEPROM_SSID.begin(), EEPROM_SSID.end(),
                  [](char c) { return c == static_cast<char>(0xFF); })) {
    Serial.println("SSID not found or invalid in EEPROM. Starting AP...");
    setupAP();
    server.on("/", handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);
    server.on("/clear-wifi", HTTP_POST, handleClearWiFi);
    server.begin();
    return;
  }

  Serial.printf("Attempting to connect to SSID: %s\n", EEPROM_SSID.c_str());
  if (connectWifi(EEPROM_SSID, EEPROM_PASSWORD)) {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    connectToMQTT();
    if (strcmp(LIZ_TYPE, "sensor") == 0) {
      environmentalSensorSetup(SENSOR_TYPE);
    } else if (strcmp(LIZ_TYPE, "voltmeter") == 0) {
      voltageSensorSetup();
    } else {
      Serial.println("Please set LIZ_TYPE");
    }

    server.on("/", webpage_status);  // Status page handler

    if (otaHelper != nullptr) {
      delete otaHelper;
    }

    otaHelper = new OTAHelper();
    otaHelper->setup();
    wizSetup();
  } else {
    setupAP();
    server.on("/", handleRoot);  // Setup page handler
  }
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/clear-wifi", HTTP_POST, handleClearWiFi);
  server.begin();
}

void blinkLED(int count) {
  static uint32_t lastToggleTime = 0;
  static int blinkCount = 0;
  static bool ledState = LOW;

  if (millis() - lastToggleTime >= LED_BLINK_DELAY) {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);

    lastToggleTime = millis();

    if (ledState == LOW) {
      blinkCount++;
      if (blinkCount >= count) {
        blinkCount = 0;
      }
    }
  }
}

bool isWizDevice(const std::string &moduleName) {
  return moduleName.find("SOCKET") != std::string::npos ||
         moduleName.find("SHRGBC") != std::string::npos;
}

void discoverAndSubscribe() {
  auto oldDiscoveredDevices = liz::getDiscoveredDevices();
  auto discoveredDevices = liz::discover(port, 60000);

  for (const auto &device : discoveredDevices) {
    if (std::find(oldDiscoveredDevices.begin(), oldDiscoveredDevices.end(),
                  device) == oldDiscoveredDevices.end()) {
      std::string deviceIP = std::get<0>(device);
      std::string deviceMac = std::get<1>(device);
      std::string moduleName = std::get<2>(device);

      if (isWizDevice(moduleName)) {
        std::string deviceName = "wiz_" + moduleName + "_" + deviceMac;
        sendMQTTWizDiscoveryMsg(deviceIP, deviceName);

        std::string stateTopic = "home/storage/" + deviceName + "/set";
        client.subscribe(stateTopic.c_str());
      }
    }
  }
}

void loop() {
  if ((WiFi.status() != WL_CONNECTED)) {
    server.handleClient();
    return;
  }

  if ((mqttStatus() == false)) {
    Serial.print("Connecting to MQTT");
    connectToMQTT();
  }

  if (otaHelper != nullptr) {
    otaHelper->loop();
  }

  handleTelnet();
  server.handleClient();
  resetWiFi.update();
  blinkLED(LED_BLINK_COUNT);
  client.loop();
  if (strcmp(LIZ_TYPE, "sensor") == 0) {
    readAndWriteEnvironmentalSensors(SENSOR_TYPE);
  } else if (strcmp(LIZ_TYPE, "voltmeter") == 0) {
    readAndWriteVoltageSensor();
  }
  // runGetPilot();
  discoverAndSubscribe();
}
