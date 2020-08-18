#ifndef EMULATORTRX_H
#define EMULATORTRX_H

#include "iradio.h"

class EmulatorTRX: public IRadio
{
  Q_OBJECT
public:
  EmulatorTRX();

public Q_SLOTS:
  void setTXFreq(int freq) override;

private:
  void timerEvent(QTimerEvent *event) override;
};

#endif // EMULATORTRX_H
