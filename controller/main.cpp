#include <QApplication>
#include <QVector>
#include <QHBoxLayout>
#include "form.h"
#include "mechpanel.h"
#include <QLabel>

#include "FlexRadio.h"
#include "MechaduinoController.h"
#include "applicaionsettings.h"

//ttyACM0
int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  Form f;
  f.show();

  QHBoxLayout* mechaduinoContainer = f.findChild<QHBoxLayout*>("mechaduinoContainer");
  if(mechaduinoContainer)
  {
    MechPanel* mp = new MechPanel();
    mechaduinoContainer->addWidget(mp);
  }
  //f.horizontalLayoutWidget.
//  QString configPath = "/home/user/Projects/RemoteTubeAmplifaier/controller/config.conf";
//  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
//  setting.loadSettings(configPath);

//  FlexRadio flexRadio(setting.getFlex6xxx_IP(), setting.getFlex6xxx_port());

//  const QVector<MechaduinoController*>& v = setting.getMechConrollerList();
//  QVector<MechaduinoController*>::const_iterator i;
//  for(i = v.begin(); i!= v.end(); ++i)
//  {
//    QObject::connect(&flexRadio, &FlexRadio::radioFrequency, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
//  }

  return a.exec();
}
