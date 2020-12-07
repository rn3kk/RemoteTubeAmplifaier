#include "applicaionsettings.h"
#include "jsonprotokol.h"

bool JsonProtokol::isRequest(const QByteArray &data)
{

}

bool JsonProtokol::checkToken(const QByteArray &data)
{
  ApplicaionSettings& settings = ApplicaionSettings::getInstance();
  QByteArray token = settings.getTokenHash();

  return false;
}


