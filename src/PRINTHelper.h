#ifndef PRINTHELPER_H
#define PRINTHELPER_H

#include <Arduino.h>
#include <WiFiClient.h>

extern WiFiClient serverClient;

class PRINTHelper
{
public:
  PRINTHelper(WiFiClient &client)
      : _client(client) {}

  void print(const String &message)
  {
    Serial.print(message);
    _client.print(message);
  }

  void println(const String &message)
  {
    Serial.println(message);
    _client.println(message);
  }

  void printf(const char *format, ...)
  {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, 256, format, args);
    va_end(args);
    Serial.println(buf);
    _client.println(buf);
  }

private:
  WiFiClient &_client;
};

#endif