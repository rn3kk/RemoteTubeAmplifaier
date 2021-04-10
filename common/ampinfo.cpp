#include <QXmlStreamWriter>
#include <QDateTime>
#include <QXmlStreamReader>
#include "common.h"
#include "ampinfo.h"

const QString IP = "ip";
const QString NAME = "name";
const QString PORT = "port";
const QString PA = "pa";
const QString TIME = "time";

AmpInfo::AmpInfo()
{

}

AmpInfo::AmpInfo(QString name, QString ip):
  m_paName(name),
  m_paIp(ip)
{
  QDateTime dt = QDateTime::currentDateTime();
  m_timeCreating = dt.toString("dd.MM.yyyy hh:mm:ss.zzz");
  m_paPort = PA_TCP_PORT;
}

AmpInfo::AmpInfo(QByteArray xml)
{
  QXmlStreamReader reader(xml);
  while (!reader.atEnd() && !reader.hasError())
  {
    QXmlStreamReader::TokenType token = reader.readNext();
    if (token == QXmlStreamReader::StartDocument)
      continue;
    if (token == QXmlStreamReader::StartElement)
    {
      if (reader.name() == PA)
        continue;
      if (reader.name() == NAME)
      {
        m_paName = reader.readElementText(QXmlStreamReader::IncludeChildElements);
        continue;
      }
      if(reader.name() == IP)
      {
        m_paIp = reader.readElementText(QXmlStreamReader::IncludeChildElements);
        continue;
      }
      if(reader.name() == PORT)
      {
        m_paPort = reader.readElementText(QXmlStreamReader::IncludeChildElements).toInt();
        continue;
      }
      if(reader.name() == TIME)
      {
        m_timeCreating = reader.readElementText(QXmlStreamReader::IncludeChildElements);
      }
    }
  }
}

AmpInfo::AmpInfo(const AmpInfo &ai)
{
  m_paIp = ai.paIp();
  m_paName = ai.paName();
  m_paPort = ai.paPort();
  m_timeCreating = ai.timeCreating();
}

AmpInfo::~AmpInfo()
{

}

QByteArray AmpInfo::toXml() const
{
  QByteArray xml;
  QXmlStreamWriter writer(&xml);
  writer.writeStartDocument();
  writer.writeStartElement(PA);
  writer.writeTextElement(NAME, m_paName);
  writer.writeTextElement(IP, m_paIp);
  writer.writeTextElement(PORT, QString::number(m_paPort));
  writer.writeTextElement(TIME, m_timeCreating);
  writer.writeEndElement();
  writer.writeEndDocument();
  return xml;
}

bool AmpInfo::isValid()
{
  if(!m_paName.isEmpty() &&
     m_paPort > 0 &&
     !m_paIp.isEmpty())
    return true;

  return false;
}

QString AmpInfo::paName() const
{
  return m_paName;
}

QString AmpInfo::paIp() const
{
  return m_paIp;
}

quint16 AmpInfo::paPort() const
{
  return m_paPort;
}

QString AmpInfo::timeCreating() const
{
  return m_timeCreating;
}
