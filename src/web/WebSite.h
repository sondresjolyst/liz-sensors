// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_WEB_WEBSITE_H_
#define SRC_WEB_WEBSITE_H_

#include <PubSubClient.h>
#include <WebServer.h>
#include <WiFi.h>

#include <vector>

extern WebServer server;
extern WiFiClient serverClient;
extern PubSubClient *mqttClient;

String getWifiOptions();
void handleRoot();
void webpage_status();
void handleSubmit();
void handleClearWiFi();

#endif  // SRC_WEB_WEBSITE_H_
