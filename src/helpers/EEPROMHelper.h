// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_EEPROMHELPER_H_
#define SRC_HELPERS_EEPROMHELPER_H_

#include <Arduino.h>

extern const int EEPROM_SSID_START;
extern const int EEPROM_SSID_END;
extern const int EEPROM_PASSWORD_START;
extern const int EEPROM_PASSWORD_END;

void EEPROMHelper_begin(size_t size);
void writeEEPROM(unsigned int start, int end, String data);
String readEEPROM(int start, int end);
void clearWifiCredentials();
void writeEEPROMInt(unsigned int start, int end, int value);
int readEEPROMInt(int start, int end);

#endif  // SRC_HELPERS_EEPROMHELPER_H_
