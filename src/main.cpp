#include <Arduino.h>
#include <DNSServer.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "arduino_secrets.h"
#include "EEPROMHelper.h"
#include "MQTTHelper.h"
#include "WebServer.h"
#include "WiFiHelper.h"
#include "DHTHelper.h"
#include "OTAHelper.h"
#include "WIZHelper.h"
#include "liz.h"
#include "PRINTHelper.h"
#include <regex>
#include "BMEHelper.h"
#include <cstring>
#include <Wire.h>

Adafruit_BME280 bme;

uint32_t chipId = ESP.getChipId();
String CHIP_ID_STRING = String(chipId, HEX);
String MQTT_HOSTNAME_STRING = "Wemos_D1_Mini_" + CHIP_ID_STRING;
const char *MQTT_HOSTNAME = MQTT_HOSTNAME_STRING.c_str();
String MQTT_STATETOPIC = "home/storage/" + String(MQTT_HOSTNAME) + "/state";
const uint8_t DHTTYPE = DHT11;
const char *MQTT_BROKER = SECRET_MQTTBROKER;
const char *MQTT_PASS = SECRET_MQTTPASS;
const char *MQTT_USER = SECRET_MQTTUSER;
const char *SENSOR = "BME"; // "BME" or "DHT"
const char *WIFI_NAME = "Fuktsensor";
const float TEMP_HUMID_DIFF = 10.0;
const int DHT_SENSOR_PIN = 2;
const int DNS_PORT = 53;
const int EEPROM_PASSWORD_END = 96;
const int EEPROM_PASSWORD_START = 32;
const int EEPROM_SSID_END = 31;
const int EEPROM_SSID_START = 0;
const int LED_BLINK_COUNT = 10;
const int LED_BLINK_DELAY = 1000;
const int MQTT_PORT = SECRET_MQTTPORT;
const int RESET_BUTTON_GPO = 4;
const int RESET_PRESS_DURATION = 5000;
const int SERIAL_PORT = 9600;
const int WEBSITE_PORT = 80;
const int WIFI_DELAY = 1000;
const int WIFI_TRIES = 15;
// const char *ipaddress = "192.168.1.203";
const int port = 38899;
const int runGetPilot_BLINK_DELAY = 2000;

DHT dht(DHT_SENSOR_PIN, DHTTYPE, 11);
ESP8266WebServer server(WEBSITE_PORT);
ResetWiFi resetWiFi(RESET_BUTTON_GPO, RESET_PRESS_DURATION);
OTAHelper *otaHelper = nullptr;
PRINTHelper printHelper(serverClient);

void setup()
{
  Serial.begin(SERIAL_PORT);
  delay(100);

  Serial.println("Disconnecting WiFi");
  WiFi.disconnect();

  EEPROM.begin(512);
  delay(10);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Starting...");
  Serial.println("Reading EEPROM...");

  String EEPROM_SSID = readEEPROM(EEPROM_SSID_START, EEPROM_SSID_END);
  String EEPROM_PASSWORD = readEEPROM(EEPROM_PASSWORD_START, EEPROM_PASSWORD_END);

  if (EEPROM_SSID.length() == 0)
  {
    Serial.println("SSID not found in EEPROM. Starting AP...");
    setupAP();
    return;
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.print(EEPROM_SSID);
  Serial.print(" with PASSWORD: ");
  Serial.print(EEPROM_PASSWORD);
  if (connectWifi(EEPROM_SSID, EEPROM_PASSWORD))
  {
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    connectToMQTT();

    if (strcmp(SENSOR, "DHT") == 0)
    {
      dht.begin();

      for (int i = 0; i < DHT_NUM_READINGS; i++)
      {
        DHTtempReadings[i] = dht.readTemperature() + DHTtempOffset;
        DHThumidReadings[i] = dht.readHumidity() + DHThumidOffset;
        DHTtotalTemp += DHTtempReadings[i];
        DHTtotalHumid += DHThumidReadings[i];
      }
    }
    else if (strcmp(SENSOR, "BME") == 0)
    {
      Serial.print("Sensor type is BME");
      Wire.begin();
      if (!bme.begin(0x76))
      {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1)
          ;
      }
      for (int i = 0; i < BME_NUM_READINGS; i++)
      {
        BMEtempReadings[i] = bme.readTemperature() + BMEtempOffset;
        BMEhumidReadings[i] = bme.readHumidity() + BMEhumidOffset;
        BMEtotalTemp += BMEtempReadings[i];
        BMEtotalHumid += BMEhumidReadings[i];
      }
    }
    else
    {
      Serial.print("Error: No sensor type selected!");
    }

    server.on("/", webpage_status);
    server.begin();

    otaHelper = new OTAHelper();
    otaHelper->setup();

    wizSetup();
  }
  else
  {
    setupAP();
  }

  server.on("/", handleRoot);
  server.on("/submit", HTTP_POST, handleSubmit);
  server.begin();
}

void blinkLED(int count)
{
  static unsigned long lastToggleTime = 0;
  static int blinkCount = 0;
  static bool ledState = LOW;

  if (millis() - lastToggleTime >= LED_BLINK_DELAY)
  {
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);

    lastToggleTime = millis();

    if (ledState == LOW)
    {
      blinkCount++;
    }
    if (blinkCount >= count)
    {
      blinkCount = 0;
    }
  }
}

void discoverAndSubscribe()
{
  // Get the current discoveredDevices
  auto oldDiscoveredDevices = liz::getDiscoveredDevices();

  // Discover devices
  auto discoveredDevices = liz::discover(port, 5000);

  // Only subscribe if a new device has been discovered
  for (const auto &device : discoveredDevices)
  {
    // Check if the device is not in oldDiscoveredDevices
    if (std::find(oldDiscoveredDevices.begin(), oldDiscoveredDevices.end(), device) == oldDiscoveredDevices.end())
    {
      std::string deviceIP = std::get<0>(device);
      std::string deviceMac = std::get<1>(device);
      std::string moduleName = std::get<2>(device);

      // Filter out "SOCKET" or "SHRGBC"
      std::smatch match;
      if (std::regex_search(moduleName, match, std::regex("(SOCKET|SHRGBC)")))
      {
        moduleName = match.str();
      }

      std::string deviceName = "wiz_" + moduleName + "_" + deviceMac;
      sendMQTTWizDiscoveryMsg(deviceIP, deviceName);

      std::string stateTopic = "home/storage/" + deviceName + "/set";
      client.subscribe(stateTopic.c_str());
      // runGetPilot(deviceIP);
    }
  }
}

void loop()
{
  if ((WiFi.status() != WL_CONNECTED))
  {
    server.handleClient();
    return;
  }

  if ((mqttStatus() == false))
  {
    Serial.print("Connecting to MQTT");
    connectToMQTT();
  }

  if (otaHelper != nullptr)
  {
    otaHelper->loop();
  }

  handleTelnet();
  server.handleClient();
  resetWiFi.update();
  blinkLED(LED_BLINK_COUNT);
  client.loop();
  if (strcmp(SENSOR, "DHT") == 0)
  {
    readAndWriteDHT();
  }
  else if (strcmp(SENSOR, "BME") == 0)
  {
    readAndWriteBME();
  }
  // runGetPilot();
  discoverAndSubscribe();
}
