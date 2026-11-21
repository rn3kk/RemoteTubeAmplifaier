#pragma once

#include "TciProtocol.h"

constexpr int TUNE_POS_MAX = 2162;
constexpr int SWITCH_POSITIONS = 5;

struct Point {
  uint16_t freqKhz;
  uint16_t cap;
  uint16_t vario;
  uint8_t sw;
  bool saved;
};

enum class ControlMode { Manual, Automatic };
enum class AutoBlock { None, NoConnection, RadioUnknown, RadioOff, NoFrequency, NoCell, NotConfigured };

struct AutoSelection {
  int index = -1;
  AutoBlock block = AutoBlock::NoConnection;
};

int frequencyCell(const Point *points, int count, uint32_t frequencyHz);
AutoSelection selectAutomaticPoint(const Point *points, int count, bool connected, const TciRadioState &radio);
const char *autoBlockText(AutoBlock block);
