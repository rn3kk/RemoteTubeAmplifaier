#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QDebug>
#include <QTimerEvent>
#include "common.h"
#include "../common/commands.h"
#include "MechaduinoController.h"
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

QByteArray BackModel::getStatus()
{
  const QByteArray& a1 =
      Commands::createMessage(CMD_PWR, m_power) +
      Commands::createMessage(CMD_TUNE_MODE, m_tuneMode)+
      Commands::createMessage(CMD_MANUAL_MODE, m_mech1_pos) +
      Commands::createMessage(CMD_MECH2, m_mech2_pos) +
      Commands::createMessage(CMD_PROTECTION_STATUS, m_extenalProtection) +
      Commands::createMessage(CMD_FREQ, m_radioFreq);

  return a1;
}

void BackModel::externalProtection(bool state)
{
  QMutexLocker ml(&m_mutex);
  m_extenalProtection = state;
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

void BackModel::mechaduinoIsChanged()
{
  MechaduinoController * controller = qobject_cast<MechaduinoController*>(sender());
  QMutexLocker ml(&m_mutex);
//  for(Mechaduino& m: m_mechaduinos)
//  {
//    if(m.name.compare(controller->getName()) == 0)
//    {
//      m.position = controller->getLastPos();
//      m.manualMode = controller->getManualMode();
//    }
//  }
  markChanged();
}

void BackModel::timerEvent(QTimerEvent *event)
{
  killTimer(event->timerId());

  if(m_isChanged)
  {
    {
      QMutexLocker ml(&m_mutex);
      QByteArray a = getStatus();
      emit modelIsChanged(getStatus());
      m_isChanged = false;
    }
  }
  startTimer(100);
}


