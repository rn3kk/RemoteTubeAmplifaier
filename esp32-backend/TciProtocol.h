#pragma once

#include <stddef.h>
#include <stdint.h>

struct TciRadioState {
  bool runningKnown = false;
  bool running = false;
  bool frequencyValid = false;
  bool modulationValid = false;
  bool transmitting = false;
  uint32_t frequencyHz = 0;
  uint32_t frequencyUpdates = 0;
  char modulation[16] = {};
};

// TCI commands may share a WebSocket message or span several fragments.
class TciProtocol {
public:
  void reset();
  void feed(const uint8_t *data, size_t length);
  const TciRadioState &state() const;

private:
  void processCommand();

  TciRadioState radio;
  char command[192] = {};
  size_t used = 0;
  bool overflow = false;
};
