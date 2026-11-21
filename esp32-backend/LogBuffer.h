#pragma once

#include <stddef.h>
#include <string.h>

// Fixed storage: old lines are replaced when the journal fills up.
class LogBuffer {
public:
  static constexpr size_t LineCount = 64;
  static constexpr size_t LineSize = 256;
  static constexpr size_t TextCapacity = LineCount * LineSize + 1;

  void append(const char *line) {
    const size_t length = strlen(line);
    const size_t stored = length < LineSize - 2 ? length : LineSize - 2;
    memcpy(lines[next], line, stored);
    // Each event occupies one line, including messages received from other tasks.
    for (size_t i = 0; i < stored; ++i) {
      if (lines[next][i] == '\r' || lines[next][i] == '\n')
        lines[next][i] = ' ';
    }
    lines[next][stored] = '\n';
    lines[next][stored + 1] = '\0';
    next = (next + 1) % LineCount;
    if (count < LineCount)
      ++count;
  }

  void copyTo(char *output) const {
    size_t offset = 0;
    const size_t first = (next + LineCount - count) % LineCount;
    for (size_t i = 0; i < count; ++i) {
      const char *line = lines[(first + i) % LineCount];
      const size_t length = strlen(line);
      memcpy(output + offset, line, length);
      offset += length;
    }
    output[offset] = '\0';
  }

private:
  char lines[LineCount][LineSize] = {};
  size_t count = 0;
  size_t next = 0;
};
