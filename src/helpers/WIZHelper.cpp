// Copyright (c) 2023-2025 Sondre Sjølyst

#include "MQTTHelper.h"
#include "WIZHelper.h"

WiFiUDP Udp;

extern PRINTHelper printHelper;

void wizSetup() {
  Udp.begin(localUdpPort);
  printHelper.log("INFO", "Now listening at IP %s, UDP port %d",
                  WiFi.localIP().toString().c_str(), localUdpPort);
}
