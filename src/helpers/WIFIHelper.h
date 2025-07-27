// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_WIFIHELPER_H_
#define SRC_HELPERS_WIFIHELPER_H_

#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "EEPROMHelper.h"

extern String WIFI_NAME;
extern const int DNS_PORT;
extern const int EEPROM_PASSWORD_END;
extern const int EEPROM_PASSWORD_START;
extern const int EEPROM_SSID_END;
extern const int EEPROM_SSID_START;
extern const int WIFI_DELAY;
extern const int WIFI_TRIES;

extern DNSServer dnsServer;
extern WiFiServer telnetServer;
extern WiFiClientSecure* secureClient;
extern WebServer server;

const size_t kBufferSize = 256;

bool connectWifi(String ssid, String password);
void handleNotFound();
void setupAP();
void handleTelnet();

class ResetWiFi {
 public:
  ResetWiFi(int pin, uint32_t duration);
  void update();

 private:
  int buttonPin;
  int32_t buttonPressTime;
  int32_t pressDuration;
};

#endif  // SRC_HELPERS_WIFIHELPER_H_
