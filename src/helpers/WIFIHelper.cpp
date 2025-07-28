// Copyright (c) 2023-2025 Sondre Sjølyst

#include <WebServer.h>

#include "WIFIHelper.h"

DNSServer dnsServer;
WiFiServer telnetServer(23);
WiFiClient telnetClient;

bool connectWifi(String ssid, String password) {
  WiFi.begin(ssid.c_str(), password.c_str());
  int tries = 0;
  while (tries < WIFI_TRIES) {
    if (WiFi.status() == WL_CONNECTED) {
      printHelper.log("INFO", "WiFi connected after %d attempts", tries + 1);
      telnetServer.begin();
      telnetServer.setNoDelay(true);
      return true;
    }
    printHelper.log("INFO", "WiFi connect attempt %d", tries + 1);
    delay(WIFI_DELAY);
    tries++;
  }
  printHelper.log("ERROR", "Could not connect to WiFi after %d attempts",
                  tries);
  return false;
}

void handleNotFound() {
  server.sendHeader("Location", "http://192.168.4.1", true);
  server.send(302, "text/plain", "");
}

void setupAP() {
  printHelper.log("INFO", "Setting up Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_NAME);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.onNotFound(handleNotFound);
  server.begin();
  printHelper.log("INFO", "Access Point is up and running!");
}
void handleTelnet() {
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      if (telnetClient)
        telnetClient.stop();
      telnetClient = telnetServer.accept();
    }
  }

  while (telnetClient.available()) {
    Serial.write(telnetClient.read());
  }

  if (Serial.available()) {
    size_t len = Serial.available();
    len = (len > kBufferSize) ? kBufferSize : len;
    uint8_t sbuf[kBufferSize];
    Serial.readBytes(sbuf, len);
    telnetClient.write(sbuf, len);
    telnetClient.clear();
    delay(1);
  }
}

ResetWiFi::ResetWiFi(int pin, uint32_t duration)
    : buttonPin(pin), buttonPressTime(0), pressDuration(duration) {
  pinMode(buttonPin, INPUT_PULLUP);
}

void ResetWiFi::update() {
  int buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) {
    buttonPressTime = 0;
    return;
  }

  if (buttonPressTime == 0) {
    printHelper.log("INFO", "Pressed!");
    buttonPressTime = millis();
    return;
  }
  printHelper.log("INFO", "Button pressed for %d ms",
                  millis() - buttonPressTime);
  if ((millis() - buttonPressTime) <= pressDuration) {
    return;
  }

  clearWifiCredentials();
  buttonPressTime = 0;
}
