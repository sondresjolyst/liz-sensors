// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_WIZHELPER_H_
#define SRC_HELPERS_WIZHELPER_H_

#include <ArduinoJson.h>
#include <WiFiUdp.h>

extern unsigned int localUdpPort;
extern WiFiUDP Udp;

void wizSetup();

#endif  // SRC_HELPERS_WIZHELPER_H_
