#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QDebug>
#include <QTimerEvent>
#include "../common/commands.h"
#include "common.h"
#include "frontmodel.h"

FrontModel::FrontModel(QObject *parent) : QObject(parent)
{

}

void FrontModel::fromJson(const QByteArray &data)
{
  QByteArray d(data);
  d = d.replace('\n', "");
  d = d.replace(' ', "");
  тутут
  int start = d.indexOf('{');

  QList<QByteArray> list = data.split('}');
  for(QByteArray a: list)
  {
    a = a.replace('\n', "");
    if(a.isNull() || a.isEmpty())
      continue;
    int cmd, value;
    if(Commands::parseMessage(a, cmd, value))
    {
      qDebug() << "Cmd" << cmd << "Value" << value;
    }
  }

  int pwr, tune_mode, manual_mode, relay, mech1, mech2;
  QMutexLocker lk(&m_mutex);
  bool res = Commands::parseStatus(data, pwr,tune_mode, manual_mode, relay, mech1, mech2);
  if(res)
  {
    m_power = pwr;
    m_tuneMode = tune_mode;
    m_mech1_pos = mech1;
    m_mech2_pos = mech2;
    markChanged();
    Q_EMIT modelChanged();
  }
}

