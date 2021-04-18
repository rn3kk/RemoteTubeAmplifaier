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

long int lastFreq=7001;
void EmulatorTRX::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());
  emit freqChanged(lastFreq++);
  startTimer(1000);
}
