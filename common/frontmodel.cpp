#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QDebug>
#include <QTimerEvent>
#include <QRegularExpression>
#include "../common/commands.h"
#include "../common/common.h"
#include "frontmodel.h"

FrontModel::FrontModel(QObject *parent) : QObject(parent)
{

}

void FrontModel::fromJson(const QByteArray &data)
{
  QByteArray d(data);
  d = d.replace('\n', "");
  d = d.replace(' ', "");
  m_data_queue.append(d);

  QRegularExpression exp(R"(\{.*?\})");
  QRegularExpressionMatchIterator iter = exp.globalMatch(m_data_queue);
  QStringList list;

  while (iter.isValid() && iter.hasNext())
    list << iter.next().captured();

  for(QString s: list)
  {
    m_data_queue = m_data_queue.replace(s, "");
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(s.toStdString().c_str(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        qDebug() << jsonError.errorString();
        continue;;
    }
    QJsonObject obj = doc.object();
    QJsonValue command = obj.value(CMD);
    QJsonValue value = obj.value(VALUE);
    int cmd = command.toInt();
    int val = value.toInt();
    switch (cmd) {
    case CMD_PWR:
      m_power = val;
      break;
    case CMD_TUNE_MODE:
      m_tuneMode = val;
      break;
    case CMD_MECH1:
      m_mech1_pos = val;
      break;
    case CMD_MECH2:
      m_mech2_pos= val;
      break;
    case CMD_RELAY:
      m_relayNumber = val;
      break;
    case CMD_FREQ:
      m_radioFreq = val;
      break;
    case CMD_PROTECTION_STATUS:
      m_extenalProtection = val;
      break;
    }
    markChanged();
    Q_EMIT modelChanged();
  }
}

