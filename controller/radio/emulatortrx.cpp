#include <QTimerEvent>
#include "emulatortrx.h"

EmulatorTRX::EmulatorTRX()
{
  startTimer(1000);
}

void EmulatorTRX::setTXFreq(int freq)
{
  emit freqChanged(freq);
}

void EmulatorTRX::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());
  emit freqChanged(7049);
}
