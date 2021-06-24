#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QDebug>
#include <QTimerEvent>
#include "common.h"
#include "backmodel.h"

BackModel::BackModel(QObject *parent):
  QObject(parent)
{
  startTimer(1000);
}

BackModel &BackModel::getInstance()
{
  static BackModel model;
  return model;
}

void BackModel::setMechaduinos(QMap<QString, int> mechaduinos)
{
  m_mechaduinoStates = mechaduinos;
}

QByteArray BackModel::toJson()
{
  QJsonObject recordObject;
  recordObject.insert(DATA_TYPE, MODEL);
  recordObject.insert(POWER, QString::number(m_power));
  recordObject.insert(FREQ, m_radioFreq);
  recordObject.insert(RELAY, m_relayNumber);
  recordObject.insert(TUNE_MODE, QString::number(m_tuneMode));

  QJsonArray mech;
  for(QString key: m_mechaduinoStates.keys())
  {
    int value = m_mechaduinoStates.value(key);
    QJsonObject mechObj;
    mechObj.insert(key, value);
    mech.push_back(mechObj);
  }
  recordObject.insert(MECH, mech);

  QJsonDocument doc(recordObject);
  return doc.toJson();
}

void BackModel::externalProtection(bool state)
{
  QMutexLocker ml(&m_mutex);
  m_extenalProtection = state;
  markChanged();
}

void BackModel::change(QPair<QString, QString> pair)
{
  QMutexLocker ml(&m_mutex);
  if(pair.first.compare(POWER) == 0)
  {
    bool pwr = QVariant(pair.second).toBool();
    if(pwr == m_power)
      return;
    else
      m_power = pwr;
  }
  else if(pair.first.compare(RELAY) == 0)
  {
    int relay = pair.second.toInt();
    if(m_relayNumber == relay)
      return;
    else
      m_relayNumber = relay;
  }
  else if(pair.first.compare(TUNE_MODE) == 0)
  {
    bool tuneMode = QVariant(pair.second).toBool();
    if(m_tuneMode == tuneMode)
      return;
    else
      m_tuneMode = tuneMode;
  }
  else if(pair.first.indexOf(MECH) == 0)
  {
    int pos = pair.second.indexOf(SEPARATOR);
    QString mechName = pair.second.left(pos-1);
    int mechPos = pair.second.right(pos+1).toInt();

    if(m_mechaduinoStates.value(mechName) == mechPos)
      return;
    else
      m_mechaduinoStates[mechName] = mechPos;
  }
  else
    return;

  markChanged();
}

void BackModel::setRadioFreq(int radioFreq)
{
  if(m_radioFreq == radioFreq)
    return;
  QMutexLocker ml(&m_mutex);
  m_radioFreq = radioFreq;
  markChanged();
}

void BackModel::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());

  if(m_isChanged)
  {
    {
      QMutexLocker ml(&m_mutex);
      QByteArray a = toJson();
      emit modelIsChanged(toJson());
      m_isChanged = false;
    }
  }
  startTimer(100);
}


