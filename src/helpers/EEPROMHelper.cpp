// Copyright (c) 2023-2025 Sondre Sjølyst

#include <EEPROM.h>
#include "EEPROMHelper.h"

static bool eeprom_initialized = false;

void EEPROMHelper_begin(size_t size) {
  if (!eeprom_initialized) {
    EEPROM.begin(size);
    eeprom_initialized = true;
  }
}

void writeEEPROM(unsigned int start, int end, String data) {
  if (!eeprom_initialized) return;
  unsigned int size = end - start;
  if (data.length() > size) {
    data = data.substring(0, size);
  }
  for (unsigned int i = 0; i < size; i++) {
    if (i < data.length()) {
      EEPROM.write(start + i, data[i]);
    } else {
      EEPROM.write(start + i, 0);
    }
  }
  EEPROM.commit();
}

String readEEPROM(int start, int end) {
  if (!eeprom_initialized) return "";
  String res = "";
  for (int i = start; i <= end; i++) {
    res += static_cast<char>(EEPROM.read(i));
  }
  return res;
}

void clearWifiCredentials() {
  if (!eeprom_initialized) return;
  for (int i = EEPROM_SSID_START; i < EEPROM_PASSWORD_END; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("Restarting ESP...");
  ESP.restart();
}
