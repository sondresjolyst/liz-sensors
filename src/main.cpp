// Copyright (c) 2023-2025 Sondre Sjølyst

#include <Arduino.h>
#include <DHT.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <regex>
#include <string>
#include <vector>

#include "./liz.h"
#include "controllers/SensorController.h"
#include "controllers/VoltmeterController.h"
#include "helpers/EEPROMHelper.h"
#include "helpers/MQTTHelper.h"
#include "helpers/OTAHelper.h"
#include "helpers/PRINTHelper.h"
#include "helpers/WIZHelper.h"
#include "helpers/WiFiHelper.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include "web/WebSite.h"

#ifndef SENSOR_TYPE
#define SENSOR_TYPE "bme"  // "bme" or "dht"
#endif

#ifndef GARGE_TYPE
#define GARGE_TYPE "sensor"  // "voltmeter" or "sensor"
#endif

const char *MQTT_BROKER = "emqx-mqtt.prod.tumogroup.com";
const int MQTT_PORT = 8883;
bool isAPMode = false;

constexpr uint8_t DHTTYPE = DHT11;
constexpr float TEMP_HUMID_DIFF = 10.0;
constexpr int DHT_SENSOR_PIN = 2;
constexpr int DNS_PORT = 53;
constexpr int EEPROM_MQTT_PASSWORD_END = 511;
constexpr int EEPROM_MQTT_PASSWORD_START = 384;
constexpr int EEPROM_MQTT_USERNAME_END = 383;
constexpr int EEPROM_MQTT_USERNAME_START = 256;
constexpr int EEPROM_PASSWORD_END = 127;
constexpr int EEPROM_PASSWORD_START = 0;
constexpr int EEPROM_SSID_END = 255;
constexpr int EEPROM_SSID_START = 128;
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
constexpr int LIGHT_PIN = 2;
volatile bool OTA_IN_PROGRESS = false;

extern const char *TOPIC_ROOT;
extern const char *TOPIC_SET;

WiFiClientSecure *secureClient = nullptr;
PubSubClient *mqttClient = nullptr;
Adafruit_BME280 bme;
DHT dht(DHT_SENSOR_PIN, DHTTYPE, 11);
WebServer server(WEBSITE_PORT);
ResetWiFi resetWiFi(RESET_BUTTON_GPO, RESET_PRESS_DURATION);
OTAHelper *otaHelper = nullptr;
PRINTHelper printHelper(secureClient);

String CHIP_ID;
String WIFI_NAME;
String EEPROM_MQTT_USERNAME;
String EEPROM_MQTT_PASSWORD;
String toLowerFunc(const char *s) {
  String tmp = String(s);
  tmp.toLowerCase();
  return tmp;
}

const char *root_ca PROGMEM = R"literal(
-----BEGIN CERTIFICATE-----
MIIFBjCCAu6gAwIBAgIRAIp9PhPWLzDvI4a9KQdrNPgwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAw
WhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3Mg
RW5jcnlwdDEMMAoGA1UEAxMDUjExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB
CgKCAQEAuoe8XBsAOcvKCs3UZxD5ATylTqVhyybKUvsVAbe5KPUoHu0nsyQYOWcJ
DAjs4DqwO3cOvfPlOVRBDE6uQdaZdN5R2+97/1i9qLcT9t4x1fJyyXJqC4N0lZxG
AGQUmfOx2SLZzaiSqhwmej/+71gFewiVgdtxD4774zEJuwm+UE1fj5F2PVqdnoPy
6cRms+EGZkNIGIBloDcYmpuEMpexsr3E+BUAnSeI++JjF5ZsmydnS8TbKF5pwnnw
SVzgJFDhxLyhBax7QG0AtMJBP6dYuC/FXJuluwme8f7rsIU5/agK70XEeOtlKsLP
Xzze41xNG/cLJyuqC0J3U095ah2H2QIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIB
hjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB
/wIBADAdBgNVHQ4EFgQUxc9GpOr0w8B6bJXELbBeki8m47kwHwYDVR0jBBgwFoAU
ebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAC
hhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcG
A1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcN
AQELBQADggIBAE7iiV0KAxyQOND1H/lxXPjDj7I3iHpvsCUf7b632IYGjukJhM1y
v4Hz/MrPU0jtvfZpQtSlET41yBOykh0FX+ou1Nj4ScOt9ZmWnO8m2OG0JAtIIE38
01S0qcYhyOE2G/93ZCkXufBL713qzXnQv5C/viOykNpKqUgxdKlEC+Hi9i2DcaR1
e9KUwQUZRhy5j/PEdEglKg3l9dtD4tuTm7kZtB8v32oOjzHTYw+7KdzdZiw/sBtn
UfhBPORNuay4pJxmY/WrhSMdzFO2q3Gu3MUBcdo27goYKjL9CTF8j/Zz55yctUoV
aneCWs/ajUX+HypkBTA+c8LGDLnWO2NKq0YD/pnARkAnYGPfUDoHR9gVSp/qRx+Z
WghiDLZsMwhN1zjtSC0uBWiugF3vTNzYIEFfaPG7Ws3jDrAMMYebQ95JQ+HIBD/R
PBuHRTBpqKlyDnkSHDHYPiNX3adPoPAcgdF3H2/W0rmoswMWgTlLn1Wu0mrks7/q
pdWfS6PJ1jty80r2VKsM/Dj3YIDfbjXKdaFU5C+8bhfJGqU3taKauuz0wHVGT3eo
6FlWkWYtbt4pgdamlwVeZEW+LM7qZEJEsMNPrfC03APKmZsJgpWCDWOKZvkZcvjV
uYkQ4omYCTX5ohy+knMjdOmdH9c7SpqEWBDC86fiNex+O0XOMEZSa8DA
-----END CERTIFICATE-----
)literal";

String productNameLower = toLowerFunc(PRODUCER_NAME);
String gargeTypeLower = toLowerFunc(GARGE_TYPE);
String sensorTypeLower = toLowerFunc(SENSOR_TYPE);
const String OTA_PRODUCT_NAME =
    productNameLower + "_" + gargeTypeLower + "_" + sensorTypeLower;

typedef unsigned char uchar;
static const char base64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static std::string base64_encode(const std::string &in) {
  std::string out;

  int val = 0, valb = -6;
  for (uchar c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(static_cast<char>(base64_table[(val >> valb) & 0x3F]));
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(
        static_cast<char>(base64_table[((val << 8) >> (valb + 8)) & 0x3F]));
  while (out.size() % 4)
    out.push_back('=');
  return out;
}

static std::string base64_decode(const std::string &in) {
  std::string out;

  std::vector<int> T(256, -1);
  for (int i = 0; i < 64; i++)
    T[static_cast<unsigned char>(base64_table[i])] = i;

  int val = 0, valb = -8;
  for (uchar c : in) {
    if (T[c] == -1)
      break;
    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back(static_cast<char>((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}

void gargeSetupAP() {
  setupAP();
  isAPMode = true;
}

void setupTime() {
  configTime(0, 0, "pool.ntp.org");
  printHelper.log("DEBUG", "Waiting for NTP time sync...");
  time_t now = time(nullptr);
  int64_t start = millis();
  const int64_t timeout = 10000;

  while ((now < 8 * 3600 * 2) && (millis() - start < timeout)) {
    delay(500);
    now = time(nullptr);
    if (WiFi.status() != WL_CONNECTED) {
      printHelper.log("WARN", "WiFi lost, attempting reconnect...");
      return;
    }
  }
  if (now < 8 * 3600 * 2) {
    printHelper.log("WARN", "NTP sync timeout. Time not set.");
  } else {
    printHelper.log("DEBUG", "NTP sync successful. Current time: %s",
                    ctime(&now));
  }
}

bool isNightTime() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  int hour = timeinfo.tm_hour;
  return (hour >= 2 && hour < 4);
}

#define WIFI_DEBUG

String getMacString() {
  uint64_t chipid = ESP.getEfuseMac();
  char macStr[13];
  snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
           (uint8_t)(chipid), (uint8_t)(chipid >> 8), (uint8_t)(chipid >> 16),
           (uint8_t)(chipid >> 24), (uint8_t)(chipid >> 32),
           (uint8_t)(chipid >> 40));
  String mac = String(macStr);
  mac.toLowerCase();
  return mac;
}

void setup() {
  delay(1000);
  Serial.begin(SERIAL_PORT);
  delay(100);

  pinMode(LIGHT_PIN, OUTPUT);

  uint32_t dummy = esp_random();
  printHelper.log("DEBUG", "Hardware RNG initialized, first value: %u", dummy);

  CHIP_ID = getMacString();
  WIFI_NAME = "Garge " + String(CHIP_ID);

  printHelper.log("DEBUG", "Chip ID: %s", CHIP_ID);

  printHelper.log("DEBUG", "Disconnecting WiFi");
  WiFi.disconnect();

  EEPROMHelper_begin(EEPROM_SIZE);
  delay(10);

  pinMode(LIGHT_PIN, OUTPUT);
  printHelper.log("DEBUG", "Starting...");
  printHelper.log("DEBUG", "Reading EEPROM...");

  String EEPROM_SSID = readEEPROM(EEPROM_SSID_START, EEPROM_SSID_END);
  String EEPROM_PASSWORD =
      readEEPROM(EEPROM_PASSWORD_START, EEPROM_PASSWORD_END);
  EEPROM_MQTT_USERNAME =
      readEEPROM(EEPROM_MQTT_USERNAME_START, EEPROM_MQTT_USERNAME_END);
  EEPROM_MQTT_PASSWORD =
      readEEPROM(EEPROM_MQTT_PASSWORD_START, EEPROM_MQTT_PASSWORD_END);

  printHelper.log("DEBUG", "EEPROM_SSID: '%s', EEPROM_PASSWORD: '%s'",
                  EEPROM_SSID.c_str(), EEPROM_PASSWORD.c_str());
  printHelper.log("DEBUG", "MQTT USER: '%s', MQTT PASS: '%s'",
                  EEPROM_MQTT_USERNAME.c_str(), EEPROM_MQTT_PASSWORD.c_str());

  if (EEPROM_SSID.isEmpty() || EEPROM_SSID.length() < 2 ||
      std::all_of(EEPROM_SSID.begin(), EEPROM_SSID.end(),
                  [](char c) { return c == static_cast<char>(0xFF); })) {
    printHelper.log("DEBUG",
                    "SSID not found or invalid in EEPROM. Starting AP...");
    gargeSetupAP();
    server.on("/", handleRoot);
    server.on("/submit", HTTP_POST, handleSubmit);
    server.on("/clear-wifi", HTTP_POST, handleClearWiFi);
    server.begin();
    return;
  }

  printHelper.log("DEBUG", "Attempting to connect to SSID: %s",
                  EEPROM_SSID.c_str());
  if (connectWifi(EEPROM_SSID, EEPROM_PASSWORD)) {
    printHelper.log("INFO", "WiFi connected, IP: %s",
                    WiFi.localIP().toString().c_str());
    printHelper.log("DEBUG", "WiFi RSSI: %d", WiFi.RSSI());

    setupTime();
    Serial.setDebugOutput(true);
    esp_log_level_set("mbedtls", ESP_LOG_DEBUG);
    secureClient = new WiFiClientSecure();
    secureClient->setCACert(root_ca);
    secureClient->setHandshakeTimeout(30);
    mqttClient = new PubSubClient(*secureClient);
    mqttClient->setServer(MQTT_BROKER, MQTT_PORT);

    if (strcmp(GARGE_TYPE, "sensor") == 0) {
      environmentalSensorSetup(SENSOR_TYPE);
    } else if (strcmp(GARGE_TYPE, "voltmeter") == 0) {
      voltageSensorSetup();
    } else {
      printHelper.log("ERROR", "Please set GARGE_TYPE");
    }

    server.on("/", webpage_status);

    if (otaHelper != nullptr) {
      delete otaHelper;
    }

    otaHelper = new OTAHelper();
    otaHelper->setup();

    otaHelper->checkAndUpdateFromManifest(OTA_MANIFEST_URL,
                                          OTA_PRODUCT_NAME.c_str(), VERSION);

    wizSetup();
  } else {
    printHelper.log("WARN", "WiFi connection failed, starting AP mode.");
    gargeSetupAP();
    server.on("/", handleRoot);
  }
  server.on("/submit", HTTP_POST, handleSubmit);
  server.on("/clear-wifi", HTTP_POST, handleClearWiFi);
  server.begin();

  printHelper.log("INFO", "%s %s started", OTA_PRODUCT_NAME.c_str(), VERSION);
}

void blinkLED(int count, int LED_BLINK_DELAY) {
  static uint32_t lastToggleTime = 0;
  static int blinkCount = 0;
  static bool ledState = LOW;

  if (millis() - lastToggleTime >= LED_BLINK_DELAY) {
    ledState = !ledState;
    digitalWrite(LIGHT_PIN, ledState);

    lastToggleTime = millis();

    if (ledState == LOW) {
      blinkCount++;
      if (blinkCount >= count) {
        blinkCount = 0;
      }
    }
  }
}

// Returns true if "SOCKET" or "SHRGBC"
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

      // Extract only "SOCKET" or "SHRGBC" from moduleName
      std::smatch match;
      std::string moduleType;
      if (std::regex_search(moduleName, match, std::regex("(SOCKET|SHRGBC)"))) {
        moduleType = match.str();
      } else {
        moduleType = "unknown";
      }

      if (isWizDevice(moduleType)) {
        std::string deviceName = "wiz_" + moduleType + "_" + deviceMac;

        publishDiscoveredDeviceConfig(deviceName.c_str(), moduleType.c_str(),
                                      "Wiz");

        publishGargeDiscoveryEvent(CHIP_ID, deviceName.c_str(),
                                   moduleType.c_str());

        String setTopic = String(TOPIC_ROOT) + deviceName.c_str() + TOPIC_SET;
        mqttClient->subscribe(setTopic.c_str());
        printHelper.log("INFO", "Subscribed to %s", setTopic.c_str());
      }
    }
  }
}

void checkSerialForCredentials() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    if (line.startsWith("SETMQTTCRED:")) {
      int firstColon = line.indexOf(':');
      int secondColon = line.indexOf(':', firstColon + 1);
      String b64_username = line.substring(firstColon + 1, secondColon);
      String b64_password = line.substring(secondColon + 1);

      std::string decoded_user =
          base64_decode(std::string(b64_username.c_str()));
      std::string decoded_pass =
          base64_decode(std::string(b64_password.c_str()));

      String username = String(decoded_user.c_str());
      String password = String(decoded_pass.c_str());

      writeEEPROM(EEPROM_MQTT_USERNAME_START, EEPROM_MQTT_USERNAME_END,
                  username);
      writeEEPROM(EEPROM_MQTT_PASSWORD_START, EEPROM_MQTT_PASSWORD_END,
                  password);

      EEPROM_MQTT_USERNAME = username;
      EEPROM_MQTT_PASSWORD = password;
      printHelper.log("INFO", "MQTT credentials saved");
    }
  }
}

void loop() {
  checkSerialForCredentials();

  if (isAPMode) {
    server.handleClient();
    blinkLED(LED_BLINK_COUNT, LED_BLINK_DELAY);
    // Restart after 30 minutes in AP mode
    static uint32_t apStartTime = 0;
    if (apStartTime == 0) {
      apStartTime = millis();
    }
    if (millis() - apStartTime > 30UL * 60UL * 1000UL) {  // 30 minutes
      printHelper.log("DEBUG", "Restarting after 30 minutes in AP mode");
      ESP.restart();
    }
    return;
  }

  if (!isAPMode) {
    blinkLED(LED_BLINK_COUNT, 5000);
  }

  if (WiFi.status() != WL_CONNECTED) {
    static int16_t lastAttempt = 0;
    if (millis() - lastAttempt > 5000) {
      printHelper.log("DEBUG", "WiFi lost, attempting reconnect...");
      String EEPROM_SSID = readEEPROM(EEPROM_SSID_START, EEPROM_SSID_END);
      String EEPROM_PASSWORD =
          readEEPROM(EEPROM_PASSWORD_START, EEPROM_PASSWORD_END);
      bool wifiResult = connectWifi(EEPROM_SSID, EEPROM_PASSWORD);
      printHelper.log("DEBUG", "WiFi reconnect result: %d, status: %d, IP: %s",
                      wifiResult, WiFi.status(),
                      WiFi.localIP().toString().c_str());
      lastAttempt = millis();
    }
    server.handleClient();
    return;
  }

  if ((mqttStatus() == false)) {
    static uint32_t lastMqttAttempt = 0;
    const uint32_t mqttReconnectDelay = 5000;  // 5 seconds
    auto isValid = [](const String &s) {
      if (s.length() == 0)
        return false;
      for (size_t i = 0; i < s.length(); ++i) {
        if ((uint8_t)s[i] != 0xFF && s[i] != '\0')
          return true;
      }
      return false;
    };
    if (isValid(EEPROM_MQTT_USERNAME) && isValid(EEPROM_MQTT_PASSWORD)) {
      if (millis() - lastMqttAttempt > mqttReconnectDelay) {
        uint32_t freeHeap = ESP.getFreeHeap();
        printHelper.log("DEBUG", "Free heap before MQTT connect: %u", freeHeap);
        if (freeHeap < 200000) {
          printHelper.log("ERROR", "Not enough heap for MQTT TLS connection. "
                                   "Skipping connect attempt.");
        } else {
          uint32_t entropy = esp_random();
          printHelper.log("DEBUG", "esp_random() before MQTT connect: %u",
                          entropy);

          if (secureClient) {
            delete secureClient;
          }
          secureClient = new WiFiClientSecure();
          secureClient->setCACert(root_ca);
          secureClient->setHandshakeTimeout(30);
          mqttClient->setClient(*secureClient);

          printHelper.log("DEBUG", "Connecting to MQTT...");
          connectToMQTT();
          printHelper.log("DEBUG", "MQTT status after connect: %d",
                          mqttStatus());
        }
        lastMqttAttempt = millis();
      }
    }
  }
  if (otaHelper != nullptr) {
    otaHelper->loop();
  }

  static int64_t lastOtaCheck = 0;
  const int64_t otaCheckInterval = 60UL * 60UL * 1000UL;  // 1 hour

  if (millis() - lastOtaCheck > otaCheckInterval) {
    lastOtaCheck = millis();
    if (isNightTime()) {
      printHelper.log("DEBUG", "Night time OTA check...");
      otaHelper->checkAndUpdateFromManifest(OTA_MANIFEST_URL,
                                            OTA_PRODUCT_NAME.c_str(), VERSION);
    }
  }

  handleTelnet();
  server.handleClient();
  resetWiFi.update();

  mqttClient->loop();

  if (strcmp(GARGE_TYPE, "sensor") == 0) {
    readAndWriteEnvironmentalSensors(SENSOR_TYPE);
  } else if (strcmp(GARGE_TYPE, "voltmeter") == 0) {
    if (!OTA_IN_PROGRESS) {
      readAndWriteVoltageSensor();
    } else {
      printHelper.log(
          "INFO", "OTA in progress, skipping voltage publish and deep sleep.");
    }
    return;
  }
  discoverAndSubscribe();
}
