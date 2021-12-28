#include <QFile>
#include <QLoggingCategory>
#include <QSettings>
#include <QCryptographicHash>
#include "MechaduinoController.h"
#include "applicaionsettings.h"

QLoggingCategory settCat("ApplicaionSettings");

const QString FLEX="flex";
const QString YAESU="yaesu";
const QString RADIO="radio";
const QString TYPE="type";
const QString SEPARATOR = "/";
const QString IP="ip";
const QString PORT="port";
const QString POINTS = "points";
const QString COUNT = "count";
const QString MECH = "mech";
const QString NAME = "name";
const QString STEP = "step";
const QString COM_PORT = "comPort";
const QString USE = "use";
const QString SERVER = "server";
const QString INIT_TOKEN = "token";

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

  m_settings->beginGroup(RADIO);
  QString radioType = m_settings->value(TYPE, "").toString();
  if(radioType.compare(::FLEX) == 0)
  {
    m_radioType = FLEX;
    m_flex6xxx_IP = m_settings->value(IP, "").toString();
    m_flex6xxx_port = m_settings->value(PORT, -1).toInt();

    if(m_flex6xxx_IP.isNull() || m_flex6xxx_IP.isEmpty())
    {
      qCCritical(settCat) << "Flex radio IP settings not found";
      return false;
    }
    if(m_flex6xxx_port == -1)
    {
      qCCritical(settCat) << "Flex radio port settings not found";
      return false;
    }
  }
  else if(radioType.compare(::YAESU) == 0)
  {
    m_radioType = YAESU;
    m_comPortName = m_settings->value(COM_PORT, "").toString();
    if(m_comPortName.isNull() || m_comPortName.isEmpty())
    {
      qCCritical(settCat) << "Yaesu radio comPort settings not found";
      return false;
    }
  }
  else
  {
    m_radioType = UNKNOWN_RADIO;
    qCCritical(settCat) << "Unknown radio type";
  }
  m_settings->endGroup();

  m_settings->beginGroup(SERVER);
  m_serverPort = m_settings->value(PORT).toInt();
  if(m_serverPort == 0)
    qCCritical(settCat) << "Server port not found";
  m_initToken = m_settings->value(INIT_TOKEN).toByteArray();
  if(m_initToken.isNull() || m_initToken.isEmpty())
  {
    QString rndStr = getRandomString();
    m_settings->setValue(INIT_TOKEN, rndStr);
  }

  m_settings->endGroup();

  QStringList keys;
  for(int i=1; i<=2; ++i)
  {
    m_settings->beginGroup(MECH+QString::number(i));
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
      if( i ==1 )
      {
        m_mech1 = new MechaduinoController(name, port, step, map);
      }
      else
        m_mech2 = m_mech1;
    }
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

ApplicaionSettings::RadioType ApplicaionSettings::getRadioType() const
{
  return m_radioType;
}

QString ApplicaionSettings::getComPortName() const
{
  return m_comPortName;
}

QString ApplicaionSettings::getToken() const
{
  return m_initToken;
}

QByteArray ApplicaionSettings::getTokenHash() const
{
  return QCryptographicHash::hash(m_initToken,QCryptographicHash::Md5);
}

QString ApplicaionSettings::getRandomString() const
{
  const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
  const int randomStringLength = 12; // assuming you want random strings of 12 characters

  QString randomString;
  for(int i=0; i<randomStringLength; ++i)
  {
    int index = qrand() % possibleCharacters.length();
    QChar nextChar = possibleCharacters.at(index);
    randomString.append(nextChar);
  }
  return randomString;
}

QString ApplicaionSettings::getPaName() const
{
  return m_paName;
}

quint16 ApplicaionSettings::getServerPort() const
{
  return m_serverPort;
}
