//#include <QCoreApplication>
#include <QApplication>
#include <QVector>
#include <QThread>
#include <server.h>
#include "radio/radiofactory.h"
#include "radio/iradio.h"
#include "MechaduinoController.h"
#include "applicaionsettings.h"
#include "../common/backmodel.h"
#include "../common/commands.h"

int main(int argc, char *argv[])
{
  qRegisterMetaType<Mechaduino>("Mechaduino");
  //QCoreApplication a(argc, argv);
  QApplication a(argc, argv);

  //QString configPath = "/home/user/Projects/RemoteTubeAmplifaier/controller/config.conf";
  QString configPath = "config.conf";
  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
  if(!setting.loadSettings(configPath))
    exit(0);

  BackModel& model = BackModel::getInstance();

  Server server;
  QThread serverThread;
  server.moveToThread(&serverThread);
  QObject::connect(&serverThread, &QThread::started, &server, &Server::doWork);
  serverThread.start();

  QObject::connect(&model, &BackModel::modelIsChanged, &server, &Server::sendToAllClients);
//  QObject::connect(&server, &Server::changeModel, &model,  &BackModel::change);

  
  IRadio* radio = RadioFactory::getRadio();
  QObject::connect(radio, &IRadio::freqChanged, &model, &BackModel::setRadioFreq);

//  const QVector<MechaduinoController*>& v = setting.getMechConrollerList();
//  QVector<MechaduinoController*>::const_iterator i;
//  for(i = v.begin(); i!= v.end(); ++i)
//  {
//    QObject::connect(*i, &MechaduinoController::changed ,&model, &BackModel::mechaduinoIsChanged);
//    QObject::connect(radio, &IRadio::freqChanged, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
//    QObject::connect(&server, &Server::clientNeedChangeMechaduino, *i, &MechaduinoController::changeProperty);
//    QObject::connect(&server, &Server::clientTuneModeChanged, *i, &MechaduinoController::tuneMode);

//  }

  return a.exec();
}

