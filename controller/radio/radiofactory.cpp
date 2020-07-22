#include "../applicaionsettings.h"
#include "ft857.h"
#include "FlexRadio.h"
//#include "iradio.h"
#include "radiofactory.h"

IRadio *RadioFactory::getRadio()
{
  ApplicaionSettings& settings = ApplicaionSettings::getInstance();

  switch (settings.getRadioType()) {
  case ApplicaionSettings::RadioType::FLEX_RADIO:
    return new FlexRadio(settings.getFlex6xxx_IP(), settings.getFlex6xxx_port());
    break;
  case ApplicaionSettings::RadioType::YEASU:
    return new ft857(settings.getComPortName());
    break;
  default:
    return nullptr;
    break;
  }
}
