#include <QMutexLocker>
#include <QTimerEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QVariant>
#include <QPair>
#include "applicaionsettings.h"
#include "../common/common.h"
#include "statemodel.h"

StateModel::StateModel(QObject *parent):
  QObject(parent),
  m_isChanged(false),
  m_needSendModel(false)
{
  QStringList mechList = ApplicaionSettings::getInstance().getMechNameList();
  for(QString str: mechList)
  {
    m_mechaduinoStates[str] = -1;
  }
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
  m_isChanged = true;
}

QByteArray StateModel::toJson()
{
  QJsonObject recordObject;
  recordObject.insert(POWER, QString::number(m_power));
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
  if(power == m_power)
    return;
  QMutexLocker ml(&m_mutex);
  m_power = power;
  markChanged();
}

void StateModel::setMechaduinoPosition(const QString &name, int pos)
{
  if(m_mechaduinoStates[name] == pos)
    return;
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

void StateModel::needChange(const QPair<QString, QString> &pair)
{
  if(pair.first.compare(POWER) == 0)
    setPower(QVariant(pair.second).toBool());
  else if(pair.first.compare(RELAY) == 0)
    setRelayPinNumber(pair.second.toInt());
  else if(pair.first.indexOf(MECH) == 0)
  {
    int pos = pair.second.indexOf(SEPARATOR);
    QString mechName = pair.second.left(pos-1);
    int mechPos = pair.second.right(pos+1).toInt();
    setMechaduinoPosition(mechName, mechPos);
  }
  else
    return;

  markChanged();
}


void StateModel::setRelayPinNumber(int relayPinNumber)
{
  if(relayPinNumber == m_relayNumber)
    return;
  QMutexLocker ml(&m_mutex);
  m_relayNumber = relayPinNumber;
  markChanged();
}

void StateModel::setRadioFreq(int radioFreq)
{
  if(radioFreq == m_radioFreq)
    return;
  QMutexLocker ml(&m_mutex);
  m_radioFreq = radioFreq;
  markChanged();
}
