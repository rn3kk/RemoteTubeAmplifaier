#include <QTimerEvent>
#include "freqwidget.h"
#include "emulatortrx.h"

EmulatorTRX::EmulatorTRX()
{
 // startTimer(1000);
  m_freqWidget = new FreqWidget();
  connect(m_freqWidget, &FreqWidget::freq, this, &EmulatorTRX::freqChanged);
  m_freqWidget->show();
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
  startTimer(30000);
}
