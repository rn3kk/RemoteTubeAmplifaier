#include <QApplication>
#include <QVector>
#include <QHBoxLayout>
#include <QLabel>
#include "vfo.h"
#include "form.h"
#include "mechpanel.h"
#include "radio/radiofactory.h"
#include "radio/FlexRadio.h"
#include "MechaduinoController.h"
#include "applicaionsettings.h"

//ttyACM0
int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  Form f;
  f.show();

  Vfo* vfoPanel = new Vfo();
  QObject::connect(&f, &Form::tuneMode, vfoPanel, &Vfo::tuneMode, Qt::QueuedConnection);
  QHBoxLayout* mechaduinoContainer = f.findChild<QHBoxLayout*>("mechaduinoContainer");
  mechaduinoContainer->addWidget(vfoPanel);

  //QString configPath = "/home/user/Projects/RemoteTubeAmplifaier/controller/config.conf";
  QString configPath = "config.conf";
  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
  if(!setting.loadSettings(configPath))
    exit(0);

  IRadio* radio = RadioFactory::getRadio();

  QObject::connect(radio, &FlexRadio::freqChanged, vfoPanel, &Vfo::vfoAChangeFreq, Qt::QueuedConnection);

  QObject::connect(&f, &Form::setFreq, radio, &IRadio::setTXFreq, Qt::QueuedConnection);//TODO debug
  QObject::connect(vfoPanel, &Vfo::tuneVFOTxToNewFreq, radio, &IRadio::setTXFreq, Qt::QueuedConnection);//TODO debug

  const QVector<MechaduinoController*>& v = setting.getMechConrollerList();
  QVector<MechaduinoController*>::const_iterator i;
  for(i = v.begin(); i!= v.end(); ++i)
  {
    MechPanel* mp = new MechPanel((*i)->getName());
    mechaduinoContainer->addWidget(mp);
    QObject::connect(mp, &MechPanel::changePosition, *i, &MechaduinoController::setPosition, Qt::QueuedConnection);
    QObject::connect(*i, &MechaduinoController::changedPosition ,mp, &MechPanel::newPosition, Qt::QueuedConnection);
    QObject::connect(radio, &IRadio::freqChanged, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
    QObject::connect(vfoPanel, &Vfo::savePosition, *i, &MechaduinoController::savePosition, Qt::QueuedConnection);
    QObject::connect(&f, &Form::tuneMode, *i, &MechaduinoController::tuneMode, Qt::QueuedConnection);
    QObject::connect(&f, &Form::tuneMode, mp, &MechPanel::tuneMode, Qt::QueuedConnection);
  }

  return a.exec();
}

