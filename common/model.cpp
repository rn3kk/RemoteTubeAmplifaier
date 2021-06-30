#include "../common/common.h"
#include "model.h"

Model::Model()
{

}

void Model::markChanged()
{
  m_isChanged = true;
}

QList<Mechaduino> Model::mechaduinos() const
{
  return m_mechaduinos;
}

bool Model::extenalProtection() const
{
  return m_extenalProtection;
}

bool Model::tuneMode() const
{
  return m_tuneMode;
}

int Model::relayNumber() const
{
  return m_relayNumber;
}

int Model::radioFreq() const
{
  return m_radioFreq;
}

bool Model::power() const
{
  return m_power;
}

Mechaduino::Mechaduino(const Mechaduino &m)
{
  name = m.name;
  position = m.position;
  manualMode = m.manualMode;
}

Mechaduino::Mechaduino()
{

}

QString Mechaduino::toString() const
{
  QString str = name+SEPARATOR+QString::number(position)+SEPARATOR+QString::number(manualMode);
  return str;
}

bool Mechaduino::fromString(QString str)
{
  QStringList list = str.split(SEPARATOR);
  if(list.count() == 3)
  {
    name = list.at(0);
    position = list.at(1).toInt();
    manualMode = list.at(2).toInt();
    return  true;
  }
  return false;
}

bool Mechaduino::isValid()
{
  if (!name.isEmpty())
    return true;

  return false;
}
