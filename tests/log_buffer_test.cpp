#include "../esp32-backend/LogBuffer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

int main() {
  LogBuffer buffer;
  char output[LogBuffer::TextCapacity];
  buffer.copyTo(output);
  assert(output[0] == '\0');
  buffer.append("first");
  buffer.append("second\r\ncontinued");
  buffer.copyTo(output);
  assert(strcmp(output, "first\nsecond  continued\n") == 0);

  for (size_t i = 0; i < 10000; ++i) {
    char line[32];
    snprintf(line, sizeof(line), "event %zu", i);
    buffer.append(line);
  }
  buffer.copyTo(output);
  assert(strncmp(output, "event 9936\n", strlen("event 9936\n")) == 0);
  assert(strstr(output, "event 9999\n") != nullptr);
  assert(strstr(output, "event 9935\n") == nullptr);
  size_t count = 0;
  for (const char *p = output; *p; ++p) {
    if (*p == '\n') ++count;
  }
  assert(count == LogBuffer::LineCount);

  char longLine[2048];
  memset(longLine, 'x', sizeof(longLine) - 1);
  longLine[sizeof(longLine) - 1] = '\0';
  for (size_t i = 0; i < LogBuffer::LineCount * 3; ++i)
    buffer.append(longLine);
  buffer.copyTo(output);
  assert(strlen(output) == LogBuffer::LineCount * (LogBuffer::LineSize - 1));
  assert(strlen(output) < sizeof(output));
  assert(output[strlen(output) - 1] == '\n');
  std::cout << "Log buffer tests passed\n";
}
