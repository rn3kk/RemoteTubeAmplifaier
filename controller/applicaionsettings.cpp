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

ApplicaionSettings::~ApplicaionSettings()
{
  if(m_settings)
    delete m_settings;
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
  m_settings = new QSettings(configPath, QSettings::IniFormat);
  m_settings->setIniCodec("UTF-8");

  m_settings->beginGroup(FLEX_6XXX_IP);
  m_flex6xxx_IP = m_settings->value(IP, "").toString();
  m_flex6xxx_port = m_settings->value(PORT, -1).toInt();
  m_settings->endGroup();

  QStringList keys;
  for(int i=1; i<=3; ++i)
  {
    m_settings->beginGroup(MECH+QString::number(i));
//    int freq = 1700; //add freq to config
//    while(1)
//    {
//      settings.setValue(QString::number(freq), "0");
//      freq+=25;
//      if(freq > 30000)
//        break;
//    }
//    settings.endGroup();
//    continue;

    QString name = m_settings->value(NAME).toString();
    QString port = m_settings->value(PORT).toString();
    float step = m_settings->value(STEP).toFloat();
    keys = m_settings->childKeys();

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
      int degress = m_settings->value(freq).toInt();
      map->insert(freq.toInt(), degress);
    }
    m_settings->endGroup();

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

void ApplicaionSettings::savePosition(QString name, QMap<int, int>* positions)
{
  if(!m_settings)
    return;

  QStringList keys;
  for(int i=1; i<=3; ++i)
  {
    m_settings->beginGroup(MECH+QString::number(i));
    QString n = m_settings->value(NAME).toString();
    if(n.compare(name) == 0)
    {
      for(QMap<int, int>::const_iterator it = positions->constBegin(); it != positions->constEnd(); ++it)
        m_settings->setValue(QString::number(it.key()), QString::number(it.value()));
    }
    m_settings->endGroup();
    m_settings->sync();
  }
}
