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
    QJsonObject mechObj = value.toObject();
    QString name = mechObj.keys().at(0);
    int position = mechObj.value(name).toInt(-1);
    m_mechaduinoStates[name] = position;
  }
  QMutexLocker lk(&m_mutex);
  markChanged();
  Q_EMIT modelChanged();
}

