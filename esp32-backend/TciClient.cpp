#include "TciClient.h"
#include "AppLog.h"

#include <Arduino.h>
#include <atomic>
#include <WebSocketsClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace {
struct Configuration {
  TciSettings settings;
  uint32_t generation = 0;
};

struct FrequencyRequest {
  uint32_t id = 0;
  uint32_t generation = 0;
  uint32_t connection = 0;
  uint32_t frequencyHz = 0;
};

QueueHandle_t configurationQueue = nullptr;
QueueHandle_t statusQueue = nullptr;
QueueHandle_t frequencyQueue = nullptr;
std::atomic<uint32_t> frequencyRevision{0};
bool taskReady = false;
Configuration requested;
// The following objects are accessed only by the TCI task.
WebSocketsClient socketClient;
TciProtocol protocol;
TciStatus status;
bool textFragment = false;
uint32_t connectionSerial = 0;
FrequencyRequest frequencyRequest;
bool frequencyActive = false;
uint32_t frequencySentAt = 0;
uint32_t frequencyBaseline = 0;

void failFrequency() {
  if (frequencyActive)
    status.frequencyState = TciFrequencyState::Failed;
  frequencyActive = false;
}

void onSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      protocol.reset();
      textFragment = false;
      status.link = TciLink::Connected;
      status.connection = ++connectionSerial;
      appLogPrintf("TCI: connected");
      // Read requests only. START/STOP are received notifications, never sent.
      socketClient.sendTXT("vfo:0,0;modulation:0;trx:0;");
      break;
    case WStype_DISCONNECTED:
      failFrequency();
      protocol.reset();
      textFragment = false;
      status.link = TciLink::Disconnected;
      appLogPrintf("TCI: disconnected");
      break;
    case WStype_ERROR:
      failFrequency();
      protocol.reset();
      textFragment = false;
      status.link = TciLink::Error;
      appLogPrintf("TCI: connection error");
      break;
    case WStype_TEXT:
      protocol.feed(payload, length);
      break;
    case WStype_FRAGMENT_TEXT_START:
      textFragment = true;
      protocol.feed(payload, length);
      break;
    case WStype_FRAGMENT_BIN_START:
      textFragment = false;
      break;
    case WStype_FRAGMENT:
      if (textFragment)
        protocol.feed(payload, length);
      break;
    case WStype_FRAGMENT_FIN:
      if (textFragment)
        protocol.feed(payload, length);
      textFragment = false;
      break;
    default:
      break;
  }
}

void runTci(void *) {
  socketClient.onEvent(onSocketEvent);
  socketClient.setReconnectInterval(5000);
  socketClient.enableHeartbeat(15000, 3000, 2);
  Configuration active;
  for (;;) {
    Configuration incoming;
    if (xQueueReceive(configurationQueue, &incoming, 0) == pdTRUE) {
      socketClient.disconnect();
      protocol.reset();
      textFragment = false;
      active = incoming;
      status = TciStatus{};
      status.generation = active.generation;
      if (active.settings.enabled) {
        status.link = TciLink::Connecting;
        appLogPrintf("TCI: connecting to %s:%u", active.settings.host, static_cast<unsigned>(active.settings.port));
        socketClient.begin(active.settings.host, active.settings.port, "/", "");
      } else
        appLogPrintf("TCI: connection disabled");
      xQueueOverwrite(statusQueue, &status);
    }
    if (active.settings.enabled)
      socketClient.loop();
    FrequencyRequest command;
    if (xQueueReceive(frequencyQueue, &command, 0) == pdTRUE) {
      frequencyActive = false;
      frequencyRequest = command;
      status.frequencyRequest = command.id;
      status.frequencyState = TciFrequencyState::Failed;
      const TciRadioState &current = protocol.state();
      if (command.id != frequencyRevision.load()) {
        status.frequencyState = TciFrequencyState::Cancelled;
      } else if (status.link == TciLink::Connected && command.generation == active.generation &&
                 command.connection == status.connection && current.runningKnown && current.running &&
                 !current.transmitting) {
        char text[64];
        snprintf(text, sizeof(text), "vfo:0,0,%lu;vfo:0,0;", static_cast<unsigned long>(command.frequencyHz));
        frequencyBaseline = current.frequencyUpdates;
        frequencySentAt = millis();
        if (socketClient.sendTXT(text)) {
          frequencyActive = true;
          status.frequencyState = TciFrequencyState::Sent;
          appLogPrintf("TCI: requested frequency %lu Hz", static_cast<unsigned long>(command.frequencyHz));
        }
      }
    }
    const TciRadioState &radio = protocol.state();
    if (frequencyActive) {
      if (frequencyRequest.id != frequencyRevision.load()) {
        status.frequencyState = TciFrequencyState::Cancelled;
        frequencyActive = false;
      } else if (!radio.runningKnown || !radio.running || millis() - frequencySentAt > 3000) {
        failFrequency();
      } else if (radio.frequencyValid && radio.frequencyUpdates != frequencyBaseline &&
                 radio.frequencyHz == frequencyRequest.frequencyHz) {
        status.frequencyState = TciFrequencyState::Confirmed;
        frequencyActive = false;
        appLogPrintf("TCI: frequency confirmed %lu Hz", static_cast<unsigned long>(radio.frequencyHz));
      }
    }
    if (radio.runningKnown && (!status.radio.runningKnown || radio.running != status.radio.running))
      appLogPrintf("TCI: radio %s", radio.running ? "on" : "off");
    if (radio.frequencyValid && (!status.radio.frequencyValid || radio.frequencyHz != status.radio.frequencyHz))
      appLogPrintf("TCI: RX1 / VFO A frequency %lu Hz", static_cast<unsigned long>(radio.frequencyHz));
    if (radio.modulationValid && (!status.radio.modulationValid || strcmp(radio.modulation, status.radio.modulation) != 0))
      appLogPrintf("TCI: modulation %s", radio.modulation);
    status.radio = protocol.state();
    xQueueOverwrite(statusQueue, &status);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
}

bool tciBegin(const TciSettings &settings) {
  configurationQueue = xQueueCreate(1, sizeof(Configuration));
  statusQueue = xQueueCreate(1, sizeof(TciStatus));
  frequencyQueue = xQueueCreate(1, sizeof(FrequencyRequest));
  if (!configurationQueue || !statusQueue || !frequencyQueue) {
    if (configurationQueue)
      vQueueDelete(configurationQueue);
    if (statusQueue)
      vQueueDelete(statusQueue);
    if (frequencyQueue)
      vQueueDelete(frequencyQueue);
    configurationQueue = nullptr;
    statusQueue = nullptr;
    frequencyQueue = nullptr;
    return false;
  }
  taskReady = xTaskCreate(runTci, "tci", 6144, nullptr, 1, nullptr) == pdPASS;
  if (!taskReady) {
    vQueueDelete(configurationQueue);
    vQueueDelete(statusQueue);
    vQueueDelete(frequencyQueue);
    configurationQueue = nullptr;
    statusQueue = nullptr;
    frequencyQueue = nullptr;
    return false;
  }
  return tciConfigure(settings);
}

bool tciConfigure(const TciSettings &settings) {
  if (!taskReady)
    return false;
  tciCancelFrequency();
  requested.settings = settings;
  ++requested.generation;
  return xQueueOverwrite(configurationQueue, &requested) == pdTRUE;
}

uint32_t tciSetFrequency(uint32_t frequencyHz) {
  const TciStatus current = tciReadStatus();
  if (!taskReady || current.link != TciLink::Connected || !current.radio.runningKnown ||
      !current.radio.running || frequencyHz == 0)
    return 0;
  FrequencyRequest command;
  command.id = frequencyRevision.fetch_add(1) + 1;
  command.generation = requested.generation;
  command.connection = current.connection;
  command.frequencyHz = frequencyHz;
  if (xQueueOverwrite(frequencyQueue, &command) != pdTRUE)
    return 0;
  return command.id;
}

void tciCancelFrequency() {
  frequencyRevision.fetch_add(1);
}

TciStatus tciReadStatus() {
  TciStatus result;
  if (!taskReady) {
    result.link = TciLink::Error;
  } else if (xQueuePeek(statusQueue, &result, 0) != pdTRUE || result.generation != requested.generation) {
    result = TciStatus{};
    result.link = requested.settings.enabled ? TciLink::Connecting : TciLink::Disabled;
  }
  return result;
}

const char *tciLinkName(TciLink link) {
  switch (link) {
    case TciLink::Connecting: return "connecting";
    case TciLink::Connected: return "connected";
    case TciLink::Disconnected: return "disconnected";
    case TciLink::Error: return "error";
    default: return "disabled";
  }
}
