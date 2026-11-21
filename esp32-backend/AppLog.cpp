#include "AppLog.h"

#include <Arduino.h>
#include <atomic>
#include <stdarg.h>
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {
LogBuffer journal;
StaticSemaphore_t mutexStorage;
SemaphoreHandle_t journalMutex = nullptr;
std::atomic<bool> recording{false};
}

void appLogBegin(bool enabled) {
  journalMutex = xSemaphoreCreateMutexStatic(&mutexStorage);
  recording.store(enabled);
}

bool appLogEnabled() {
  return recording.load();
}

void appLogSetEnabled(bool enabled) {
  recording.store(enabled);
}

void appLogPrintf(const char *format, ...) {
  char message[224];
  va_list arguments;
  va_start(arguments, format);
  vsnprintf(message, sizeof(message), format, arguments);
  va_end(arguments);
  const unsigned long now = millis();
  const unsigned long seconds = now / 1000;
  char line[LogBuffer::LineSize];
  snprintf(line, sizeof(line), "[%lu:%02lu:%02lu.%03lu] %s", seconds / 3600,
           seconds / 60 % 60, seconds % 60, now % 1000, message);
  if (recording.load() && journalMutex && xSemaphoreTake(journalMutex, 0) == pdTRUE) {
    if (recording.load())
      journal.append(line);
    xSemaphoreGive(journalMutex);
  }
  // Serial output remains available independently of web recording.
  Serial.print(message);
  if (message[0] == '\0' || message[strlen(message) - 1] != '\n')
    Serial.println();
}

bool appLogSnapshot(char *output) {
  if (!journalMutex || xSemaphoreTake(journalMutex, 0) != pdTRUE)
    return false;
  journal.copyTo(output);
  xSemaphoreGive(journalMutex);
  return true;
}
