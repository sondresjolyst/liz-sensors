// Copyright (c) 2023-2025 Sondre Sjølyst

#ifndef SRC_HELPERS_OTAHELPER_H_
#define SRC_HELPERS_OTAHELPER_H_

#include "PRINTHelper.h"

extern PRINTHelper printHelper;

class OTAHelper {
 public:
  OTAHelper();
  void setup();
  void loop();
  void checkAndUpdateFromManifest(const char *manifestUrl,
                                  const char *deviceName,
                                  const char *currentVersion);
};

#endif  // SRC_HELPERS_OTAHELPER_H_
