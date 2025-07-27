// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_LOGHELPER_H_
#define SRC_HELPERS_LOGHELPER_H_

#include <Arduino.h>

#define LOG(level, fmt, ...)                                                   \
  Serial.printf("[%s] " fmt "\n", level, ##__VA_ARGS__)

#endif  // SRC_HELPERS_LOGHELPER_H_
