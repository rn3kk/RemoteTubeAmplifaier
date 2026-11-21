#include "ControlLogic.h"

int frequencyCell(const Point *points, int count, uint32_t frequencyHz) {
  int result = -1;
  uint64_t bestDistance = 12501;
  for (int i = 0; i < count; ++i) {
    const uint64_t center = static_cast<uint64_t>(points[i].freqKhz) * 1000;
    const uint64_t distance = frequencyHz > center ? frequencyHz - center : center - frequencyHz;
    if (distance <= 12500 && (distance < bestDistance ||
        (distance == bestDistance && result >= 0 && points[i].freqKhz > points[result].freqKhz))) {
      result = i;
      bestDistance = distance;
    }
  }
  return result;
}

AutoSelection selectAutomaticPoint(const Point *points, int count, bool connected, const TciRadioState &radio) {
  AutoSelection result;
  if (!connected)
    return result;
  if (!radio.runningKnown) {
    result.block = AutoBlock::RadioUnknown;
    return result;
  }
  if (!radio.running) {
    result.block = AutoBlock::RadioOff;
    return result;
  }
  if (!radio.frequencyValid) {
    result.block = AutoBlock::NoFrequency;
    return result;
  }
  result.index = frequencyCell(points, count, radio.frequencyHz);
  if (result.index < 0) {
    result.block = AutoBlock::NoCell;
    return result;
  }
  const Point &point = points[result.index];
  result.block = point.saved && point.sw >= 1 && point.sw <= SWITCH_POSITIONS &&
                 point.cap <= TUNE_POS_MAX && point.vario <= TUNE_POS_MAX
                   ? AutoBlock::None : AutoBlock::NotConfigured;
  return result;
}

const char *autoBlockText(AutoBlock block) {
  switch (block) {
    case AutoBlock::NoConnection: return "нет связи с TCI";
    case AutoBlock::RadioUnknown: return "состояние трансивера неизвестно";
    case AutoBlock::RadioOff: return "трансивер выключен";
    case AutoBlock::NoFrequency: return "частота трансивера не получена";
    case AutoBlock::NoCell: return "для частоты трансивера нет ячейки";
    case AutoBlock::NotConfigured: return "ячейка частоты не настроена полностью";
    default: return nullptr;
  }
}
