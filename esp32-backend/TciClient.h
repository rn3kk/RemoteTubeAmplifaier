#pragma once

#include "TciProtocol.h"

struct TciSettings {
  char host[16] = {};
  uint16_t port = 50001;
  bool enabled = false;
};

enum class TciLink { Disabled, Connecting, Connected, Disconnected, Error };

struct TciStatus {
  uint32_t generation = 0;
  TciLink link = TciLink::Disabled;
  TciRadioState radio;
};

bool tciBegin(const TciSettings &settings);
bool tciConfigure(const TciSettings &settings);
TciStatus tciReadStatus();
const char *tciLinkName(TciLink link);
