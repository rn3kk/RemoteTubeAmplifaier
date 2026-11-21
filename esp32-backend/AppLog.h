#pragma once

#include "LogBuffer.h"

void appLogBegin(bool enabled);
void appLogSetEnabled(bool enabled);
bool appLogEnabled();
void appLogPrintf(const char *format, ...) __attribute__((format(printf, 1, 2)));
// Caller supplies at least LogBuffer::TextCapacity bytes. No waiting for a busy journal.
bool appLogSnapshot(char *output);
