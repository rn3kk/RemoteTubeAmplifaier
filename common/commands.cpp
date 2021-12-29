#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "../common/common.h"
#include "../common/model.h"
#include "commands.h"

QByteArray Commands::createMessage(int cmd, int value)
{
    QJsonObject obj;
    obj.insert(CMD, cmd);
    obj.insert(VALUE, value);
    QJsonDocument doc(obj);
    return doc.toJson();
}

bool Commands::parseMessage(const QByteArray &data, int &cmd, int &value)
{
  QJsonParseError jsonError;
  QJsonDocument doc = QJsonDocument::fromJson(data, &jsonError);
  if (jsonError.error != QJsonParseError::NoError)
  {
      qDebug() << jsonError.errorString();
      return false;
  }
  QJsonObject obj = doc.object();
  QJsonValue command = obj.value(CMD);
  QJsonValue val = obj.value(VALUE);
  if(command.isNull() || val.isNull())
    return false;
  cmd = command.toInt();
  value = val.toInt();
  return true;
}
