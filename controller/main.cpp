#include <QCoreApplication>
#include <QVector>
#include <QThread>
#include <server.h>
#include "radio/radiofactory.h"
#include "radio/iradio.h"
#include "MechaduinoController.h"
#include "applicaionsettings.h"
#include "broadcastinformer.h"
#include "../common/backmodel.h"


int main(int argc, char *argv[])
{
  qRegisterMetaType<QPair<QString, QString>>("QPair<QString, QString>");
  QCoreApplication a(argc, argv);

  //QString configPath = "/home/user/Projects/RemoteTubeAmplifaier/controller/config.conf";
  QString configPath = "config.conf";
  ApplicaionSettings& setting = ApplicaionSettings::getInstance();
  if(!setting.loadSettings(configPath))
    exit(0);

  BroadcastInformer bi;
  QThread th;
  bi.moveToThread(&th);
  QObject::connect(&th, &QThread::started, &bi, &BroadcastInformer::start);
  th.start();

  BackModel& model = BackModel::getInstance();
  QMap<QString, int> mechaduinos;
  mechaduinos["Tune"] = 10;
  model.addMechaduinos(mechaduinos); // заглушка

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
//    QObject::connect(*i, &MechaduinoController::changedPosition ,&model, &StateModel::needChange);
//    QObject::connect(radio, &IRadio::freqChanged, *i, &MechaduinoController::changeFreq, Qt::QueuedConnection);
//    QObject::connect(&model, &StateModel::tuneMode, *i, &MechaduinoController::tuneMode);
//  }

  return a.exec();
}

