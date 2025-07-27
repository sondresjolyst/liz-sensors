// Copyright (c) 2023-2025 Sondre Sjølyst

#include <WiFiClientSecure.h>

#include <cstdio>

#include "PRINTHelper.h"

PRINTHelper::PRINTHelper(WiFiClientSecure* client) : _client(client) {}

void PRINTHelper::print(const String &message) {
  Serial.print(message);
  if (_client)
    _client->print(message);
}

void PRINTHelper::println(const String &message) {
  Serial.println(message);
  if (_client)
    _client->println(message);
}

void PRINTHelper::printf(const char *format, ...) {
  char buf[256];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);
  Serial.println(buf);
  if (_client)
    _client->println(buf);
}
