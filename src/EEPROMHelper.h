// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_EEPROMHELPER_H_
#define SRC_EEPROMHELPER_H_

#include <Arduino.h>

extern const int EEPROM_SSID_START;
extern const int EEPROM_SSID_END;
extern const int EEPROM_PASSWORD_START;
extern const int EEPROM_PASSWORD_END;

void writeEEPROM(unsigned int start, int end, String data);
String readEEPROM(int start, int end);
void clearWifiCredentials();

#endif  // SRC_EEPROMHELPER_H_
