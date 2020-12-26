#include <QMutexLocker>
#include <QTimerEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QVariant>
#include <QPair>
#include "statemodel.h"

const QString FREQ = "f";
const QString POWER = "p";
const QString RELAY = "r";
const QString MECH = "m";

StateModel::StateModel(QObject *parent):
  QObject(parent),
  m_isChanged(false)
{
  startTimer(1000);
}

StateModel &StateModel::getInstance()
{
  static StateModel model;
  return model;
}

StateModel::~StateModel()
{

}

void StateModel::markChanged()
{
  QMutexLocker ml(&m_mutex);
  m_isChanged = true;
}

void StateModel::unmarkChanged()
{
  QMutexLocker ml(&m_mutex);
  m_isChanged = false;
}

QByteArray StateModel::toJson()
{
  QMutexLocker ml(&m_mutex);
  QJsonObject recordObject;
  recordObject.insert(POWER, m_power);
  recordObject.insert(FREQ, m_radioFreq);
  recordObject.insert(RELAY, m_relayNumber);

  QJsonObject mech;
  for(QString key: m_mechaduinoStates.keys())
  {
    int value = m_mechaduinoStates.value(key);
    mech.insert(key, value);
  }
  recordObject.insert(MECH, mech);

  QJsonDocument doc(recordObject);
  return doc.toJson();
}

void StateModel::setPower(bool power)
{
  QMutexLocker ml(&m_mutex);
  m_power = power;
  markChanged();
}

void StateModel::setMechaduinoPosition(const QString &name, int pos)
{
  QMutexLocker ml(&m_mutex);
  m_mechaduinoStates[name]=pos;
  markChanged();
}

void StateModel::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());

  if(m_isChanged)
  {
    {
      QMutexLocker ml(&m_mutex);
      QByteArray a = toJson();
      emit modelChanged(a);
    }
    unmarkChanged();
  }

  startTimer(100);
}

void StateModel::setRelayPinNumber(int relayPinNumber)
{
  QMutexLocker ml(&m_mutex);
  m_relayNumber = relayPinNumber;
}

void StateModel::setRadioFreq(const QString &radioFreq)
{
  QMutexLocker ml(&m_mutex);
  m_radioFreq = radioFreq;
}
