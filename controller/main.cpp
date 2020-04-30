#include <QCoreApplication>
#include <QVector>
#include "FlexRadio.h"
#include "MechaduinoController.h"
#include "applicaionsettings.h"

//ttyACM0
int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  QString configPath = "/home/user/Projects/RemoteTubeAmplifaier/controller/config.conf";
  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
  setting.loadSettings(configPath);

//  MechaduinoCommunicator communicator("/dev/ttyACM0");
//  communicator.getPosition();

  FlexRadio flexRadio(setting.getFlex6xxx_IP(), setting.getFlex6xxx_port());

  const QVector<MechaduinoController*>& v = setting.getMechConrollerList();
  QVector<MechaduinoController*>::const_iterator i;
  for(i = v.begin(); i!= v.end(); ++i)
  {
    QObject::connect(&flexRadio, &FlexRadio::radioFrequency, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
  }

//  while(1)
//  {
//    for(int i =0; i< 20; i+=1)
//    {
//      communicator.setPosition(i);
//    }
//    for(int i =80; i> 45; i-=1)
//    {
//      communicator.setPosition(i);
//    }
//    for(int i =250; i< 360; i+=2)
//    {
//      communicator.setPosition(i);
//    }
//  }
  //MainController controller("/home/user/Projects/RemoteTubeAmplifaier/controller/points");

  //CapacitorsPositionController capController ("tty1", "tty2");

  return a.exec();
}
