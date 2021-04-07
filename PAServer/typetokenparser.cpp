#include <QJsonDocument>
#include <QLoggingCategory>
#include "typetokenparser.h"

QLoggingCategory tokenCat("TypeToken");

const QString TYPE = "type";
const QString TOKEN = "token";
const QString TYPE_PA = "pa";
const QString TYPE_SOFT = "soft";
const QString TYPE_UNKN = "unkn";

TypeToken::TypeToken()
{

}

TypeToken::TypeToken(const TypeToken &token)
{
  m_token = token.m_token;
  m_type = token.m_type;
}

TypeToken::~TypeToken()
{
  qCDebug(tokenCat) << "~Token()";
}

bool TypeToken::parseTokenAndClientType(QByteArray data)
{
  QJsonDocument doc = QJsonDocument::fromJson(data);
  QString type = doc[TYPE].toString();
  QString token = doc[TOKEN].toString();
  qCDebug(tokenCat) << "TYPE=" << type << "Toke=" <<token;
  if (type.compare(TYPE_PA) == 0)
    m_type = PA_CLIENT;
  else if(type.compare(TYPE_SOFT) == 0)
    m_type = SOFT_CLIENT;
  else
  {
    qCCritical(tokenCat) << "Unknown client type:" << type;
    m_type = UNKN;
  }

  m_token = token.toUtf8();
  if(!m_token.isEmpty() && type != UNKN)
  {
    qCInfo(tokenCat) << " New connection with Type" << type << "Token" << token;
    return true;
  }
  return false;
}

QByteArray TypeToken::token() const
{
  return m_token;
}

TypeToken::Type TypeToken::type() const
{
  return m_type;
}

QString TypeToken::getTypeString() const
{
  switch (m_type) {
  case PA_CLIENT:
    return TYPE_PA;
  case SOFT_CLIENT:
    return TYPE_SOFT;
  default:
    return TYPE_UNKN;
  }
}
