// Copyright (c) 2023-2025 Sondre Sjølyst

#include "WIZHelper.h"
#include "MQTTHelper.h"
#include "PRINTHelper.h"

unsigned int localUdpPort = 38899;
WiFiUDP Udp;

extern PRINTHelper printHelper;

void wizSetup() {
  Udp.begin(localUdpPort);
  printHelper.printf("Now listening at IP %s, UDP port %d\n",
                     WiFi.localIP().toString().c_str(), localUdpPort);
}
