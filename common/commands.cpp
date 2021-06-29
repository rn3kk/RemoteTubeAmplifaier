#include <QJsonDocument>
#include <QJsonObject>
//#include "applicaionsettings.h"
#include "../common/common.h"
#include "../common/model.h"
#include "commands.h"

bool Commands::isRequest(const QByteArray &data)
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

QByteArray Commands::createChangeRequest(QString key, QString newValue)
{
  QJsonObject recordObject;
  recordObject.insert(REQUEST, CHANGE_REUQEST);
  recordObject.insert(KEY, key);
  recordObject.insert(VALUE, newValue);
  QJsonDocument doc(recordObject);
  return doc.toJson();
}

QPair<QString, QString> Commands::parceChangeRequest(QByteArray data)
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

Commands::Type Commands::getType(const QByteArray &data)
{
  QJsonDocument d = QJsonDocument::fromJson(data);
  QJsonObject o = d.object();
  QString request = o[REQUEST].toString();
  if(request.isNull() || request.isEmpty())
    return UNKW;
  else if(request.compare(::CHANGE_POWER) == 0)
    return CHANGE_POWER;
  else if(request.compare(::CHANGE_TUNE) == 0)
    return CHANGE_TUNE;
  else if(request.compare(::CHANGE_RELAY) == 0)
    return CHANGE_RELAY;
  else if(request.compare(::CHANGE_MECH) == 0)
    return CHANGE_MECH;
  else
    return UNKW;
}

QString Commands::getValue(const QByteArray &data)
{
  QJsonDocument d = QJsonDocument::fromJson(data);
  QJsonObject o = d.object();
  QString val = o[VALUE].toString();
  return val;
}

Mechaduino Commands::getMechaduinoProperty(const QByteArray &data)
{
  QByteArray mech = getValue(data).toUtf8();
  Mechaduino m;
  if(m.fromString(mech))
  {
    return m;
  }
  else
    return Mechaduino();

}

QByteArray Commands::changePwr(bool pwr)
{
  return createReq(::CHANGE_POWER, QString::number(pwr));
}

QByteArray Commands::changeTune(bool tune)
{
  return createReq(::CHANGE_TUNE, QString::number(tune));
}

QByteArray Commands::changeRelay(int relay)
{
  return createReq(::CHANGE_POWER, QString::number(relay));
}

QByteArray Commands::changeMech(QString mechString)
{
  return createReq(::CHANGE_MECH, mechString);
}

QByteArray Commands::createReq(QString req, QString value)
{
  QJsonObject recordObject;
  recordObject.insert(REQUEST, req);
  recordObject.insert(VALUE, value);
  QJsonDocument doc(recordObject);
  return doc.toJson();
}
