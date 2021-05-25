#include <QJsonDocument>
#include <QJsonObject>
//#include "applicaionsettings.h"
#include "../common/common.h"
#include "jsonprotokol.h"

bool JsonProtokol::isRequest(const QByteArray &data)
{
  QJsonDocument d = QJsonDocument::fromJson(data);
  QJsonObject o = d.object();
  QJsonValue v = o.value(REQUEST);
  QString get = v.toString();
  if(get.compare(REQUEST_GET_INFO) == 0)
    return true;

  return false;
}

//bool JsonProtokol::checkToken(const QByteArray &data)
//{
//  ApplicaionSettings& settings = ApplicaionSettings::getInstance();
//  QString token = settings.getToken();

//  QJsonDocument d = QJsonDocument::fromJson(data);
//  QJsonObject o = d.object();
//  QJsonValue v = o.value(TOKEN);
//  QString token_str = v.toString();
//  if(token_str.compare(token) == 0)
//    return true;

//  return false;
//}

QByteArray JsonProtokol::createChangeRequest(QString key, QString newValue)
{
  QJsonObject recordObject;
  recordObject.insert(REQUEST, CHANGE_REUQEST);
  recordObject.insert(KEY, key);
  recordObject.insert(VALUE, newValue);
  QJsonDocument doc(recordObject);
  return doc.toJson();
}

QPair<QString, QString> JsonProtokol::parceChangeRequest(QByteArray data)
{
  QJsonDocument d = QJsonDocument::fromJson(data);
  QJsonObject o = d.object();
  QString request = o[REQUEST].toString();
  QString key = o[KEY].toString();
  QString value = o[VALUE].toString();
  if(request.compare(CHANGE_REUQEST) == 0)
  {
       QPair<QString, QString> pair(key, value);
       return pair;
  }
  return QPair<QString, QString>();
}
