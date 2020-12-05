#include <QMutexLocker>
#include <QTimerEvent>
#include "statemodel.h"

StateModel::StateModel(QObject *parent):
  QObject(parent),
  m_isChanged(false)
{
  startTimer(1000);
}

void StateModel::markChanged()
{
  QMutexLocker ml(&m_mutex);
  m_isChanged = true;
}

void StateModel::setPower(bool power)
{
  QMutexLocker ml(&m_mutex);
  m_power = power;
  markChanged();
}

void StateModel::mechaduinoPosition(const QString &name, int pos)
{
  QMutexLocker ml(&m_mutex);
  m_mechaduinoStates[name]=pos;
  markChanged();
}

void StateModel::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());

  if(m_isChanged)
    Х

  startTimer(100);
}
