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

#include "./config/arduino_secrets.h"
#include "./liz.h"
#include "controllers/SensorController.h"
#include "controllers/VoltmeterController.h"
#include "helpers/EEPROMHelper.h"
#include "helpers/MQTTHelper.h"
#include "helpers/OTAHelper.h"
#include "helpers/PRINTHelper.h"
#include "helpers/WIZHelper.h"
#include "helpers/WiFiHelper.h"
#include "web/WebSite.h"

#ifndef SENSOR_TYPE
#define SENSOR_TYPE "bme"  // "bme" or "dht"
#endif

#ifndef GARGE_TYPE
#define GARGE_TYPE "sensor"  // "voltmeter" or "sensor"
#endif

const char *MQTT_BROKER = SECRET_MQTTBROKER;
const char *MQTT_PASS = SECRET_MQTTPASS;
const char *MQTT_USER = SECRET_MQTTUSER;
const int MQTT_PORT = SECRET_MQTTPORT;
bool isAPMode = false;

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
WiFiClient serverClient;
PRINTHelper printHelper(&serverClient);

String CHIP_ID_STRING;
String MQTT_HOSTNAME_STRING;
const char *MQTT_HOSTNAME = nullptr;
String MQTT_STATETOPIC;
String WIFI_NAME;

String toLowerFunc(const char *s) {
  String tmp = String(s);
  tmp.toLowerCase();
  return tmp;
}

String producerNameLower = toLowerFunc(PRODUCER_NAME);  // garge
String gargeTypeLower = toLowerFunc(GARGE_TYPE);        // voltmeter, sensor
String sensorTypeLower = toLowerFunc(SENSOR_TYPE);      // bme, dht
const String OTA_PRODUCT_NAME =
    producerNameLower + "_" + gargeTypeLower + "_" + sensorTypeLower;

void gargeSetupAP() {
  setupAP();
  isAPMode = true;
}

void setupTime() {
  configTime(0, 0, "pool.ntp.org");
  Serial.print("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  int64_t start = millis();
  const int64_t timeout = 10000;  // 10 seconds

  while ((now < 8 * 3600 * 2) && (millis() - start < timeout)) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(
          "\nWiFi disconnected during NTP sync. Aborting time setup.");
      return;
    }
  }
  if (now < 8 * 3600 * 2) {
    Serial.println("\nNTP sync timeout. Time not set.");
  } else {
    Serial.println(" done!");
  }
}

bool isNightTime() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  int hour = timeinfo.tm_hour;
  return (hour >= 2 && hour < 4);
}

void setup() {
  Serial.begin(SERIAL_PORT);
  delay(100);

  uint64_t chipId = ESP.getEfuseMac();
  CHIP_ID_STRING = String(chipId, HEX);
  MQTT_HOSTNAME_STRING = producerNameLower + "_" + CHIP_ID_STRING;
  MQTT_HOSTNAME = MQTT_HOSTNAME_STRING.c_str();
  MQTT_STATETOPIC = "garge/devices/sensors/" + MQTT_HOSTNAME_STRING + "/" +
                    MQTT_HOSTNAME_STRING + "/state";
  WIFI_NAME = "Garge " + String(CHIP_ID_STRING);

  Serial.println("Disconnecting WiFi");
  WiFi.disconnect();

  EEPROMHelper_begin(EEPROM_SIZE);
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
    gargeSetupAP();
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

    setupTime();

    connectToMQTT();
    if (strcmp(GARGE_TYPE, "sensor") == 0) {
      environmentalSensorSetup(SENSOR_TYPE);
    } else if (strcmp(GARGE_TYPE, "voltmeter") == 0) {
      voltageSensorSetup();
    } else {
      Serial.println("Please set GARGE_TYPE");
    }

    server.on("/", webpage_status);  // Status page handler

    if (otaHelper != nullptr) {
      delete otaHelper;
    }

    otaHelper = new OTAHelper();
    otaHelper->setup();

    otaHelper->checkAndUpdateFromManifest(OTA_MANIFEST_URL,
                                          OTA_PRODUCT_NAME.c_str(), VERSION);

    wizSetup();
  } else {
    gargeSetupAP();
    server.on("/", handleRoot);  // Setup page handler
  }
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/clear-wifi", HTTP_POST, handleClearWiFi);
  server.begin();

  Serial.println(OTA_PRODUCT_NAME + " " + VERSION + " started");
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

// Filter out "SOCKET" or "SHRGBC"
bool isWizDevice(const std::string &moduleName) {
  return moduleName.find("SOCKET") != std::string::npos ||
         moduleName.find("SHRGBC") != std::string::npos;
}

void discoverAndSubscribe() {
  // Get the current discoveredDevices
  auto oldDiscoveredDevices = liz::getDiscoveredDevices();
  // Discover devices
  auto discoveredDevices = liz::discover(port, 60000);

  // Only subscribe if a new device has been discovered
  for (const auto &device : discoveredDevices) {
    // Check if the device is not in oldDiscoveredDevices
    if (std::find(oldDiscoveredDevices.begin(), oldDiscoveredDevices.end(),
                  device) == oldDiscoveredDevices.end()) {
      std::string deviceIP = std::get<0>(device);
      std::string deviceMac = std::get<1>(device);
      std::string moduleName = std::get<2>(device);

      if (isWizDevice(moduleName)) {
        std::string deviceName = "wiz_" + moduleName + "_" + deviceMac;
        sendMQTTSocketDiscoveryMsg(deviceIP, deviceName);

        std::string stateTopic = "garge/devices/sockets/" + std::string(MQTT_HOSTNAME_STRING.c_str()) + "/" + deviceName + "/set";
        client.subscribe(stateTopic.c_str());
      }
    }
  }
}

void loop() {
  if (isAPMode) {
    server.handleClient();
    // Restart after 30 minutes in AP mode
    static uint32_t apStartTime = 0;
    if (apStartTime == 0) {
      apStartTime = millis();
    }
    if (millis() - apStartTime > 30UL * 60UL * 1000UL) {  // 30 minutes
      Serial.println("Restarting after 30 minutes in AP mode");
      ESP.restart();
    }
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    static int16_t lastAttempt = 0;
    if (millis() - lastAttempt > 5000) {
      Serial.println("WiFi lost, attempting reconnect...");
      String EEPROM_SSID = readEEPROM(EEPROM_SSID_START, EEPROM_SSID_END);
      String EEPROM_PASSWORD =
          readEEPROM(EEPROM_PASSWORD_START, EEPROM_PASSWORD_END);
      connectWifi(EEPROM_SSID, EEPROM_PASSWORD);
      lastAttempt = millis();
    }
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

  static int64_t lastOtaCheck = 0;
  const int64_t otaCheckInterval = 60UL * 60UL * 1000UL;  // 1 hour

  if (millis() - lastOtaCheck > otaCheckInterval) {
    lastOtaCheck = millis();
    if (isNightTime()) {
      otaHelper->checkAndUpdateFromManifest(OTA_MANIFEST_URL,
                                            OTA_PRODUCT_NAME.c_str(), VERSION);
    }
  }

  handleTelnet();
  server.handleClient();
  resetWiFi.update();
  blinkLED(LED_BLINK_COUNT);
  client.loop();
  if (strcmp(GARGE_TYPE, "sensor") == 0) {
    readAndWriteEnvironmentalSensors(SENSOR_TYPE);
  } else if (strcmp(GARGE_TYPE, "voltmeter") == 0) {
    // Only run once per wakeup for voltmeter, then deep sleep is handled in
    // controller
    readAndWriteVoltageSensor();
    return;
  }
  discoverAndSubscribe();
}
