// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_PRINTHELPER_H_
#define SRC_HELPERS_PRINTHELPER_H_

#include <Arduino.h>
#include <WiFiClient.h>
#include <cstdio>

class PRINTHelper {
 public:
  explicit PRINTHelper(WiFiClient *client);

  void print(const String &message);
  void println(const String &message);
  void printf(const char *format, ...);

 private:
  WiFiClient *_client;
};

#endif  // SRC_HELPERS_PRINTHELPER_H_
