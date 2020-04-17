#ifndef CAPACITORSPOSITIONCONTROLLER_H
#define CAPACITORSPOSITIONCONTROLLER_H

#include <QObject>
#include "OneFrequencyPoint.h"

class CapacitorsPositionController: public QObject
{
  Q_OBJECT
public:
  CapacitorsPositionController(const QString& comPortTuneStepper, const QString& comPortLoadStepper);

public Q_SLOTS:
  void changePositionTo(const OneFrequencyPoint& point);

private:
  const QString m_comPortTuneStepper;
  const QString m_comPortLoadStepper;
};

#endif // CAPACITORSPOSITIONCONTROLLER_H
