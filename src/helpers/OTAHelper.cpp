// Copyright (c) 2023-2025 Sondre Sjølyst

#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <cstdio>

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

int versionCompare(const char* v1, const char* v2) {
    int maj1, min1, pat1;
    int maj2, min2, pat2;
    sscanf(v1, "v%d.%d.%d", &maj1, &min1, &pat1);
    sscanf(v2, "v%d.%d.%d", &maj2, &min2, &pat2);

    if (maj1 != maj2) return maj1 - maj2;
    if (min1 != min2) return min1 - min2;
    return pat1 - pat2;
}

void OTAHelper::checkAndUpdateFromManifest(const char *manifestUrl,
                                           const char *deviceName,
                                           const char *currentVersion) {
  HTTPClient http;
  http.begin(manifestUrl);
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("Failed to fetch manifest: %d\n", httpCode);
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("Failed to parse manifest JSON");
    return;
  }

  const char *latest_version = nullptr;
  const char *latest_bin_url = nullptr;

  for (JsonObject entry : doc.as<JsonArray>()) {
    const char *name = entry["name"];
    const char *version = entry["version"];
    const char *bin_url = entry["bin_url"];
    if (name && version && bin_url && strcmp(name, deviceName) == 0) {
      if (!latest_version || versionCompare(version, latest_version) > 0) {
          latest_version = version;
          latest_bin_url = bin_url;
      }
    }
  }

  if (!latest_version || !latest_bin_url) {
    Serial.println("No matching device or missing fields in manifest");
    return;
  }

  if (strcmp(latest_version, currentVersion) == 0) {
    Serial.println("Already up to date");
    return;
  }

  Serial.printf("New version available: %s\n", latest_version);
  Serial.println("Starting OTA update...");

  http.begin(latest_bin_url);
  int binCode = http.GET();
  if (binCode != 200) {
    Serial.printf("Failed to fetch bin: %d\n", binCode);
    http.end();
    return;
  }

  int contentLength = http.getSize();
  bool canBegin = Update.begin(contentLength);
  if (!canBegin) {
    Serial.println("Not enough space for OTA");
    http.end();
    return;
  }

  WiFiClient *stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written == contentLength && Update.end()) {
    Serial.println("OTA Success! Rebooting...");
    ESP.restart();
  } else {
    Serial.println("OTA Failed!");
  }
  http.end();
}
