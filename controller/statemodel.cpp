#include <QMutexLocker>
#include <QTimerEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QVariant>
#include <QPair>
#include "../common/common.h"
#include "statemodel.h"



StateModel::StateModel(QObject *parent):
  QObject(parent),
  m_isChanged(false),
  m_needSendModel(false)
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

void StateModel::fromJson(QByteArray json)
{
  QJsonDocument jsonResponse = QJsonDocument::fromJson(json);
  QJsonObject jsonObject = jsonResponse.object();
  QString freq = jsonObject[FREQ].toString();
  bool power = jsonObject[POWER].toBool();
  int relay = jsonObject[RELAY].toInt();
}

void StateModel::sendModel()
{
  QMutexLocker ml(&m_mutex);
  m_needSendModel = true;
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

  if(m_isChanged || m_needSendModel)
  {
    {
      QMutexLocker ml(&m_mutex);
      QByteArray a = toJson();
      emit modelChanged(a);
      m_isChanged = false;
      m_needSendModel = false;
    }
  }

  startTimer(100);
}

bool StateModel::getConnected() const
{
  return m_connected;
}

void StateModel::setConnected(bool connected)
{
  m_connected = connected;
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
