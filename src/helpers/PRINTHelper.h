// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_PRINTHELPER_H_
#define SRC_HELPERS_PRINTHELPER_H_

#include <WiFiClientSecure.h>

#include <Arduino.h>
#include <cstdio>

class PRINTHelper {
 public:
  explicit PRINTHelper(WiFiClientSecure *client);

  void log(const char *level, const char *format, ...);

 private:
  WiFiClientSecure *_client;
};

#endif  // SRC_HELPERS_PRINTHELPER_H_
