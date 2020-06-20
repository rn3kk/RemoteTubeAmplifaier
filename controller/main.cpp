#include <QApplication>
#include <QVector>
#include <QHBoxLayout>
#include <QLabel>
#include "vfo.h"
#include "form.h"
#include "mechpanel.h"


#include "FlexRadio.h"
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
  \
  FlexRadio flexRadio(setting.getFlex6xxx_IP(), setting.getFlex6xxx_port());
  QObject::connect(&flexRadio, &FlexRadio::changeTXFreq, vfoPanel, &Vfo::vfoAChangeFreq, Qt::QueuedConnection);

  QObject::connect(&f, &Form::setFreq, &flexRadio, &FlexRadio::setTXFreq, Qt::QueuedConnection);//TODO debug
  QObject::connect(vfoPanel, &Vfo::tuneVFOTxToNewFreq, &flexRadio, &FlexRadio::setTXFreq, Qt::QueuedConnection);//TODO debug

  const QVector<MechaduinoController*>& v = setting.getMechConrollerList();
  QVector<MechaduinoController*>::const_iterator i;
  for(i = v.begin(); i!= v.end(); ++i)
  {
    MechPanel* mp = new MechPanel((*i)->getName());
    mechaduinoContainer->addWidget(mp);
    QObject::connect(mp, &MechPanel::changePosition, *i, &MechaduinoController::setPosition, Qt::QueuedConnection);
    QObject::connect(*i, &MechaduinoController::changedPosition ,mp, &MechPanel::newPosition, Qt::QueuedConnection);
    QObject::connect(&flexRadio, &FlexRadio::changeTXFreq, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
    QObject::connect(vfoPanel, &Vfo::savePosition, *i, &MechaduinoController::savePosition, Qt::QueuedConnection);
    QObject::connect(&f, &Form::tuneMode, *i, &MechaduinoController::tuneMode, Qt::QueuedConnection);
    QObject::connect(&f, &Form::tuneMode, mp, &MechPanel::tuneMode, Qt::QueuedConnection);
  }

  return a.exec();
}

