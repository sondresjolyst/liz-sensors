// Copyright (c) 2023-2025 Sondre Sjølyst

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "OTAHelper.h"

void onStart() { Serial.println("Start"); }

void onEnd() { Serial.println("\nEnd"); }

void onProgress(unsigned int progress, unsigned int total) {
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void onError(ota_error_t error) {
  Serial.printf("Error [%u]: ", error);
  if (error == OTA_AUTH_ERROR)
    Serial.println("Auth Failed");
  else if (error == OTA_BEGIN_ERROR)
    Serial.println("Begin Failed");
  else if (error == OTA_CONNECT_ERROR)
    Serial.println("Connect Failed");
  else if (error == OTA_RECEIVE_ERROR)
    Serial.println("Receive Failed");
  else if (error == OTA_END_ERROR)
    Serial.println("End Failed");
}

OTAHelper::OTAHelper() {}

void OTAHelper::setup() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: No WiFi connection when setting up OTAHelper");
    return;
  }

  ArduinoOTA.onStart(onStart);
  ArduinoOTA.onEnd(onEnd);
  ArduinoOTA.onProgress(onProgress);
  ArduinoOTA.onError(onError);

  ArduinoOTA.begin();
  Serial.println("OTA is ready");
}

void OTAHelper::loop() { ArduinoOTA.handle(); }
