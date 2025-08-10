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

void onStart() { printHelper.log("INFO", "OTA Start"); }

void onEnd() { printHelper.log("INFO", "OTA End"); }

void onProgress(unsigned int progress, unsigned int total) {
  printHelper.log("INFO", "OTA Progress: %u%%", (progress / (total / 100)));
}

void onError(ota_error_t error) {
  printHelper.log("ERROR", "OTA Error [%u]: ", error);
  if (error == OTA_AUTH_ERROR)
    printHelper.log("ERROR", "Auth Failed");
  else if (error == OTA_BEGIN_ERROR)
    printHelper.log("ERROR", "Begin Failed");
  else if (error == OTA_CONNECT_ERROR)
    printHelper.log("ERROR", "Connect Failed");
  else if (error == OTA_RECEIVE_ERROR)
    printHelper.log("ERROR", "Receive Failed");
  else if (error == OTA_END_ERROR)
    printHelper.log("ERROR", "End Failed");
}

OTAHelper::OTAHelper() {}

void OTAHelper::setup() {
  if (WiFi.status() != WL_CONNECTED) {
    printHelper.log("ERROR", "No WiFi connection when setting up OTAHelper");
    return;
  }

  ArduinoOTA.onStart(onStart);
  ArduinoOTA.onEnd(onEnd);
  ArduinoOTA.onProgress(onProgress);
  ArduinoOTA.onError(onError);

  ArduinoOTA.begin();
  printHelper.log("INFO", "OTA is ready");
}

void OTAHelper::loop() { ArduinoOTA.handle(); }

int versionCompare(const char *v1, const char *v2) {
  int maj1, min1, pat1;
  int maj2, min2, pat2;
  sscanf(v1, "v%d.%d.%d", &maj1, &min1, &pat1);
  sscanf(v2, "v%d.%d.%d", &maj2, &min2, &pat2);

  if (maj1 != maj2)
    return maj1 - maj2;
  if (min1 != min2)
    return min1 - min2;
  return pat1 - pat2;
}

void OTAHelper::checkAndUpdateFromManifest(const char *manifestUrl,
                                           const char *deviceName,
                                           const char *currentVersion) {
  OTA_IN_PROGRESS = true;

  HTTPClient http;
  http.begin(manifestUrl);
  int httpCode = http.GET();
  if (httpCode != 200) {
    printHelper.log("ERROR", "Failed to fetch manifest: %d", httpCode);
    http.end();
    OTA_IN_PROGRESS = false;
    return;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    printHelper.log("ERROR", "Failed to parse manifest JSON");
    OTA_IN_PROGRESS = false;
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
    printHelper.log("ERROR",
                    "No matching device or missing fields in manifest");
    OTA_IN_PROGRESS = false;
    return;
  }

  if (strcmp(latest_version, currentVersion) == 0) {
    printHelper.log("INFO", "Already up to date");
    OTA_IN_PROGRESS = false;
    return;
  }

  printHelper.log("INFO", "New version available: %s", latest_version);
  printHelper.log("INFO", "Starting OTA update...");

  http.begin(latest_bin_url);
  int binCode = http.GET();
  if (binCode != 200) {
    printHelper.log("ERROR", "Failed to fetch bin: %d", binCode);
    http.end();
    OTA_IN_PROGRESS = false;
    return;
  }

  int contentLength = http.getSize();
  bool canBegin = Update.begin(contentLength);
  if (!canBegin) {
    printHelper.log("ERROR", "Not enough space for OTA");
    http.end();
    OTA_IN_PROGRESS = false;
    return;
  }

  WiFiClient *stream = http.getStreamPtr();
  size_t written = Update.writeStream(*stream);

  if (written == contentLength && Update.end()) {
    printHelper.log("INFO", "OTA Success! Rebooting...");
    ESP.restart();
  } else {
    printHelper.log("ERROR", "OTA Failed!");
    OTA_IN_PROGRESS = false;
  }
  http.end();
}
