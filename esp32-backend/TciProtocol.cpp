#include "TciProtocol.h"

#include <ctype.h>
#include <string.h>

namespace {
char *trim(char *text) {
  while (isspace(static_cast<unsigned char>(*text)))
    ++text;
  char *end = text + strlen(text);
  while (end > text && isspace(static_cast<unsigned char>(end[-1])))
    --end;
  *end = '\0';
  return text;
}

bool parseFrequency(const char *text, uint32_t &frequency) {
  if (!*text)
    return false;
  uint32_t value = 0;
  for (; *text; ++text) {
    if (*text < '0' || *text > '9')
      return false;
    const unsigned digit = *text - '0';
    if (value > (UINT32_MAX - digit) / 10)
      return false;
    value = value * 10 + digit;
  }
  if (value == 0)
    return false;
  frequency = value;
  return true;
}
}

void TciProtocol::reset() {
  radio = TciRadioState{};
  used = 0;
  overflow = false;
}

const TciRadioState &TciProtocol::state() const {
  return radio;
}

void TciProtocol::feed(const uint8_t *data, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    const unsigned char ch = data[i];
    if (ch == ';') {
      if (!overflow) {
        command[used] = '\0';
        processCommand();
      }
      used = 0;
      overflow = false;
    } else if (!overflow) {
      if (ch == '\0' || used == sizeof(command) - 1)
        overflow = true;
      else
        command[used++] = static_cast<char>(tolower(ch));
    }
  }
}

void TciProtocol::processCommand() {
  char *name = trim(command);
  char *args = strchr(name, ':');
  if (!args) {
    if (strcmp(name, "start") == 0) {
      radio.runningKnown = true;
      radio.running = true;
    } else if (strcmp(name, "stop") == 0) {
      radio.runningKnown = true;
      radio.running = false;
      radio.transmitting = false;
      radio.frequencyValid = false;
      radio.modulationValid = false;
    }
    return;
  }
  *args++ = '\0';
  name = trim(name);
  char *values[4] = {};
  size_t count = 0;
  while (args && count < 4) {
    char *next = strchr(args, ',');
    if (next)
      *next++ = '\0';
    values[count++] = trim(args);
    args = next;
  }
  if (args || count < 2 || strcmp(values[0], "0") != 0)
    return;

  if (strcmp(name, "vfo") == 0 && count == 3 && strcmp(values[1], "0") == 0) {
    uint32_t frequency = 0;
    if (parseFrequency(values[2], frequency)) {
      radio.frequencyHz = frequency;
      ++radio.frequencyUpdates;
      radio.frequencyValid = true;
    }
  } else if (strcmp(name, "trx") == 0 && count == 2) {
    if (strcmp(values[1], "true") == 0)
      radio.transmitting = true;
    else if (strcmp(values[1], "false") == 0)
      radio.transmitting = false;
  } else if (strcmp(name, "modulation") == 0 && count == 2) {
    const size_t length = strlen(values[1]);
    if (length == 0 || length >= sizeof(radio.modulation))
      return;
    for (size_t i = 0; i < length; ++i) {
      const unsigned char ch = values[1][i];
      if (!isalnum(ch) && ch != '_')
        return;
    }
    for (size_t i = 0; i <= length; ++i)
      radio.modulation[i] = static_cast<char>(toupper(static_cast<unsigned char>(values[1][i])));
    radio.modulationValid = true;
  }
}
