#include <QCoreApplication>
#include <QWidget>
#include "FlexRadio.h"
#include "CapacitorsPositionController.h"
#include "OneFrequencyPoint.h"
#include "MainController.h"
#include "MechaduinoCommunicator.h"
#include "applicaionsettings.h"

//ttyACM0
int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  QWidget w;
  qRegisterMetaType<OneFrequencyPoint>();

  QString configPath = "/home/user/Projects/RemoteTubeAmplifaier/controller/config.conf";
  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
  setting.loadSettings(configPath);
  MechaduinoCommunicator communicator("/dev/ttyACM0");
  communicator.getPosition();

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

  //FlexRadio flexRadio("192.168.0.1", 4992);
  //CapacitorsPositionController capController ("tty1", "tty2");

  return a.exec();
}
