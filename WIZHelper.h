#ifndef WIZHELPER_H
#define WIZHELPER_H

#include <WiFiUdp.h>
#include <vector>
#include <ArduinoJson.h>
#include "PRINTHelper.h"
#include "MQTTHelper.h"

extern WiFiClient serverClient;
extern PRINTHelper printHelper;

const int WIZDISCOVER_READ_DELAY = 5000;
unsigned int localUdpPort = 38899;

WiFiUDP Udp;

std::vector<String> discoveredDevices;

void wizDiscover() {
  static unsigned long lastToggleTime = 0;

  if (millis() - lastToggleTime >= WIZDISCOVER_READ_DELAY) {
    lastToggleTime = millis();

    Udp.beginPacketMulticast(IPAddress(255, 255, 255, 255), localUdpPort, WiFi.localIP());
    Udp.write("{\"method\":\"getDevInfo\"}");
    Udp.endPacket();

    int packetSize = Udp.parsePacket();
    if (packetSize) {
      char incomingPacket[255];
      int len = Udp.read(incomingPacket, 255);
      if (len > 0) {
        incomingPacket[len] = 0;
      }
      String deviceIP = Udp.remoteIP().toString();
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, incomingPacket);
      String deviceMac = doc["result"]["devMac"];
      if (std::find(discoveredDevices.begin(), discoveredDevices.end(), deviceIP) == discoveredDevices.end()) {
        discoveredDevices.push_back(deviceIP);
        printHelper.printf("Discovered new device at IP: %s\n", deviceIP.c_str());
        String deviceName = "wiz_light_" + deviceMac;
        sendMQTTWizDiscoveryMsg(deviceIP, deviceName);
      }
    }
  }
}

void wizSetup() {
  Udp.begin(localUdpPort);
  printHelper.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}

#endif