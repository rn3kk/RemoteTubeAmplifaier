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
  QObject::connect(&flexRadio, &FlexRadio::vfoAFreq, vfoPanel, &Vfo::vfoAChangeFreq, Qt::QueuedConnection);
  QObject::connect(&flexRadio, &FlexRadio::vfoBFreq, vfoPanel, &Vfo::vfoBChangeFreq, Qt::QueuedConnection);
  QObject::connect(&flexRadio, &FlexRadio::vfoAActive, vfoPanel, &Vfo::vfoAChangeActive, Qt::QueuedConnection);
  QObject::connect(&flexRadio, &FlexRadio::vfoBActive, vfoPanel, &Vfo::vfoBChangeActive, Qt::QueuedConnection);
  QObject::connect(&flexRadio, &FlexRadio::vfoATX, vfoPanel, &Vfo::vfoATX, Qt::QueuedConnection);
  QObject::connect(&flexRadio, &FlexRadio::vfoBTX, vfoPanel, &Vfo::vfoBTX, Qt::QueuedConnection);

  QObject::connect(&f, &Form::setFreq, &flexRadio, &FlexRadio::setTXFreq, Qt::QueuedConnection);//TODO debug
  QObject::connect(vfoPanel, &Vfo::tuneTxSliceToNewFreq, &flexRadio, &FlexRadio::setTXFreq, Qt::QueuedConnection);//TODO debug

  const QVector<MechaduinoController*>& v = setting.getMechConrollerList();
  QVector<MechaduinoController*>::const_iterator i;
  for(i = v.begin(); i!= v.end(); ++i)
  {
    MechPanel* mp = new MechPanel((*i)->getName());
    mechaduinoContainer->addWidget(mp);
    //    QObject::connect(&flexRadio, &FlexRadio::vfoAFreq, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
    QObject::connect(mp, &MechPanel::changePosition, *i, &MechaduinoController::setPosition, Qt::QueuedConnection);
    QObject::connect(&f, &Form::setPosition, *i, &MechaduinoController::setPosition, Qt::QueuedConnection);
  }

  return a.exec();
}
