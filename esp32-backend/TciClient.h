#pragma once

#include "TciProtocol.h"

struct TciSettings {
  char host[16] = {};
  uint16_t port = 50001;
  bool enabled = false;
};

enum class TciLink { Disabled, Connecting, Connected, Disconnected, Error };
enum class TciFrequencyState { None, Sent, Confirmed, Failed, Cancelled };

struct TciStatus {
  uint32_t generation = 0;
  uint32_t connection = 0;
  uint32_t frequencyRequest = 0;
  TciFrequencyState frequencyState = TciFrequencyState::None;
  TciLink link = TciLink::Disabled;
  TciRadioState radio;
};

bool tciBegin(const TciSettings &settings);
bool tciConfigure(const TciSettings &settings);
uint32_t tciSetFrequency(uint32_t frequencyHz);
void tciCancelFrequency();
TciStatus tciReadStatus();
const char *tciLinkName(TciLink link);
