#include <QCoreApplication>
#include "FlexRadio.h"
#include "CapacitorsPositionController.h"
#include "OneFrequencyPoint.h"
#include "MainController.h"

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  qRegisterMetaType<OneFrequencyPoint>();

  MainController controller("/home/user/Projects/RemoteTubeAmplifaier/controller/points");

  //FlexRadio flexRadio("192.168.0.1", 4992);
  //CapacitorsPositionController capController ("tty1", "tty2");

  return a.exec();
}
