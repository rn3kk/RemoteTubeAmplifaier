#include "TciClient.h"
#include "AppLog.h"

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace {
struct Configuration {
  TciSettings settings;
  uint32_t generation = 0;
};

QueueHandle_t configurationQueue = nullptr;
QueueHandle_t statusQueue = nullptr;
bool taskReady = false;
Configuration requested;
// The following objects are accessed only by the TCI task.
WebSocketsClient socketClient;
TciProtocol protocol;
TciStatus status;
bool textFragment = false;

void onSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      protocol.reset();
      textFragment = false;
      status.link = TciLink::Connected;
      appLogPrintf("TCI: connected");
      // Read requests only. START/STOP are received notifications, never sent.
      socketClient.sendTXT("vfo:0,0;modulation:0;");
      break;
    case WStype_DISCONNECTED:
      protocol.reset();
      textFragment = false;
      status.link = TciLink::Disconnected;
      appLogPrintf("TCI: disconnected");
      break;
    case WStype_ERROR:
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
    const TciRadioState &radio = protocol.state();
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
  if (!configurationQueue || !statusQueue) {
    if (configurationQueue)
      vQueueDelete(configurationQueue);
    if (statusQueue)
      vQueueDelete(statusQueue);
    configurationQueue = nullptr;
    statusQueue = nullptr;
    return false;
  }
  taskReady = xTaskCreate(runTci, "tci", 6144, nullptr, 1, nullptr) == pdPASS;
  if (!taskReady) {
    vQueueDelete(configurationQueue);
    vQueueDelete(statusQueue);
    configurationQueue = nullptr;
    statusQueue = nullptr;
    return false;
  }
  return tciConfigure(settings);
}

bool tciConfigure(const TciSettings &settings) {
  if (!taskReady)
    return false;
  requested.settings = settings;
  ++requested.generation;
  return xQueueOverwrite(configurationQueue, &requested) == pdTRUE;
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
