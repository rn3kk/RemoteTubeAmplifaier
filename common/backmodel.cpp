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

void BackModel::addMechaduinos(QMap<QString, int> mechaduinos)
{
  m_mechaduinoStates = mechaduinos;
  markChanged();
}

void BackModel::setPwr(bool pwr)
{
  QMutexLocker ml(&m_mutex);
  m_power = pwr;
  markChanged();
}

void BackModel::setTuneMode(bool tm)
{
  qDebug()<<"setTuneMode to " << tm;
  QMutexLocker ml(&m_mutex);
  m_tuneMode = tm;
  markChanged();
}

void BackModel::setRelay(int relay)
{
  QMutexLocker ml(&m_mutex);
  m_relayNumber = relay;
  markChanged();
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

void BackModel::changeMechaduino(QPair<QString, int> pair)
{
  QMutexLocker ml(&m_mutex);
  if(m_mechaduinoStates.contains(pair.first))
  {
    m_mechaduinoStates[pair.first] = pair.second;
  }
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


