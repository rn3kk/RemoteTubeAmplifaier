#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QDebug>
#include <QTimerEvent>
#include "common.h"
#include "frontmodel.h"

FrontModel::FrontModel(QObject *parent) : QObject(parent)
{

}

void FrontModel::fromJson(const QByteArray &data)
{
  QMutexLocker lk(&m_mutex);
  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
  if (jsonError.error != QJsonParseError::NoError)
  {
    qDebug() << jsonError.errorString();
    return;
  }
  QJsonObject obj = doc.object();

  QJsonValue val = obj.value(FREQ);
  if(!val.isNull())
    m_radioFreq = val.toInt();

  val = obj.value(POWER);
  if(!val.isNull())
    m_power = QVariant(val.toString()).toBool();

  val = obj.value(TUNE_MODE);
  if(!val.isNull())
    m_tuneMode = QVariant(val.toString()).toBool();

  QJsonArray mechArr = obj[MECH].toArray();
  for(const QJsonValue value: mechArr)
  {
    Mechaduino mechaduino;
    if(mechaduino.fromString(value.toString()))
    {     
      bool found = false;
      for(Mechaduino& m : m_mechaduinos)
      {
        if(m.name.compare(mechaduino.name) == 0)
        {
          m.position = mechaduino.position;
          m.manualMode = mechaduino.manualMode;
          found = true;
          break;
        }
      }
      if(!found)
        m_mechaduinos.append(mechaduino);
    }
  }
  markChanged();
  Q_EMIT modelChanged();
}

