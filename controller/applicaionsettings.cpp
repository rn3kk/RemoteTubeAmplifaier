#include <QFile>
#include <QLoggingCategory>
#include <QSettings>
#include "MechaduinoController.h"
#include "applicaionsettings.h"

QLoggingCategory settCat("ApplicaionSettings");

const QString FLEX_6XXX_IP="FLEX6xxx";
const QString SEPARATOR = "/";
const QString IP="ip";
const QString PORT="port";
const QString POINTS = "points";
const QString COUNT = "count";
const QString MECH = "mech";
const QString NAME = "name";
const QString STEP = "step";


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

  QStringList keys;
  for(int i=1; i<=10; ++i)
  {
    settings.beginGroup(MECH+QString::number(i));
    QString name = settings.value(NAME).toString();
    QString port = settings.value(PORT).toString();
    float step = settings.value(STEP).toFloat();
    keys = settings.childKeys();

    keys.removeOne(NAME);
    keys.removeOne(PORT);
    keys.removeOne(STEP);

    QMap<int, int> * map = new QMap<int, int>();
    if(map == nullptr)
    {
      qCCritical(settCat) << "Can't allocate memory for points";
      return false;
    }
    for(QString freq: keys)
    {
      int degress = settings.value(freq).toInt();
      map->insert(freq.toInt(), degress);
    }
    settings.endGroup();

    if(!name.isNull() && !name.isEmpty() &&
       !port.isNull() && !port.isEmpty() &&
       step != 0)
    {
      m_mechConrollerList.append(new MechaduinoController(name, port, step, map));
    }
  }

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

QVector<MechaduinoController *> ApplicaionSettings::getMechConrollerList() const
{
  return m_mechConrollerList;
}
