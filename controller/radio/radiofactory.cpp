#include "../applicaionsettings.h"
#include "yaesu.h"
#include "flex.h"
#include "emulatortrx.h"
#include "radiofactory.h"

IRadio *RadioFactory::getRadio()
{
  return new EmulatorTRX();

  ApplicaionSettings& settings = ApplicaionSettings::getInstance();

  switch (settings.getRadioType()) {
  case ApplicaionSettings::RadioType::FLEX:
    return new Flex(settings.getFlex6xxx_IP(), settings.getFlex6xxx_port());
    break;
  case ApplicaionSettings::RadioType::YAESU:
    return new Yaesu(settings.getComPortName());
    break;
  default:
    return nullptr;
    break;
  }
}
