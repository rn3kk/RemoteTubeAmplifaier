#include <QFile>
#include <QLoggingCategory>
#include <QSettings>
#include "applicaionsettings.h"

QLoggingCategory settCat("ApplicaionSettings");

const QString FLEX_6XXX_IP="FLEX6xxx";
const QString SEPARATOR = "/";
const QString IP="ip";
const QString PORT="port";
const QString POINTS = "points";
const QString COUNT = "count";

ApplicaionSettings::ApplicaionSettings()
{

}

ApplicaionSettings &ApplicaionSettings::getInstance()
{
  static ApplicaionSettings settings;
  return settings;
}

bool ApplicaionSettings::loadSettings(const QString &configPath)
{
  QFile settingsFile(configPath);

  if (!settingsFile.exists())
  {
    return false;
  }
  QSettings settings(configPath, QSettings::IniFormat);
  settings.setIniCodec("UTF-8");

  settings.beginGroup(FLEX_6XXX_IP);
  m_flex6xxx_IP = settings.value(IP, "").toString();
  m_flex6xxx_port = settings.value(PORT, -1).toInt();
  settings.endGroup();

  settings.beginGroup(POINTS);
  QStringList keys = settings.childKeys();
  if(keys.isEmpty())
  {
    qCWarning(settCat) << "points array is empty";
  }
  for(QString key: keys)
  {
    OneFrequencyPoint point;
    QString pointString = settings.value(key).toString();
    if(point.fromString(pointString))
    {
      m_points.insert(point.freq, point);
    }
  }
  settings.endGroup();

  if(m_flex6xxx_IP.isNull() || m_flex6xxx_IP.isEmpty())
  {
    qCCritical(settCat)<< FLEX_6XXX_IP+SEPARATOR+IP << "bad section";
    return false;
  }
  if(m_flex6xxx_port == -1)
  {
    qCCritical(settCat)<< FLEX_6XXX_IP+SEPARATOR+PORT << "bad section";
    return false;
  }

  return true;
}

QString ApplicaionSettings::getFlex6xxx_IP() const
{
  return m_flex6xxx_IP;
}

int ApplicaionSettings::getFlex6xxx_port() const
{
  return m_flex6xxx_port;
}

QMap<int, OneFrequencyPoint> ApplicaionSettings::getPoints() const
{
  return m_points;
}
