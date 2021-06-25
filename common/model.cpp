#include "model.h"

Model::Model()
{

}

void Model::markChanged()
{
  m_isChanged = true;
}

QMap<QString, int> Model::mechaduinoStates() const
{
  return m_mechaduinoStates;
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
