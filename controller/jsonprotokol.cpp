#include <QJsonDocument>
#include <QJsonObject>
#include "applicaionsettings.h"
#include "jsonprotokol.h"

static QString TOKEN = "token";

bool JsonProtokol::isRequest(const QByteArray &data)
{

}

bool JsonProtokol::checkToken(const QByteArray &data)
{
  ApplicaionSettings& settings = ApplicaionSettings::getInstance();
  QString token = settings.getToken();

  QJsonDocument d = QJsonDocument::fromJson(data);
  QJsonObject o = d.object();
  QJsonValue v = o.value(TOKEN);
  QString token_str = v.toString();
  if(token_str.compare(token) == 0)
    return true;

  return false;
}
